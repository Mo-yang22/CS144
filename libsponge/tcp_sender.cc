#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
// libsponge\tcp_sender.cc
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
   : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
   , initial_retransmission_timeout_{retx_timeout}
   , retransmission_timeout_(retx_timeout)
   , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return bytes_in_flight_; }

void TCPSender::fill_window() {
   TCPSegment tcp_segment;
   // 如果发送完了
   if (fin_) {
       return;
   }
   // 如果还没有开始
   if (!syn_) {
       tcp_segment.header().syn = true;
       send_tcp_segment(tcp_segment);
       syn_ = true;
       return;
   }
   // 窗口大小，未知则假设为1
   uint16_t window_size = (receiver_window_size_ > 0 ? receiver_window_size_ : 1);
   // 发送结束，单独返回一个FIN包
   if (_stream.eof() && _recv_seqno + window_size > _next_seqno) {
       tcp_segment.header().fin = true;
       send_tcp_segment(tcp_segment);
       fin_ = true;
       return;
   }
   // 循环发送，直到无新的字节需要读取或者无可用空间
   while (!_stream.buffer_empty() && _recv_seqno + window_size > _next_seqno) {
       // 根据大小来读取数据
       size_t send_size =
           min(TCPConfig::MAX_PAYLOAD_SIZE, static_cast<size_t>(window_size - (_next_seqno - _recv_seqno)));
       tcp_segment.payload() = _stream.read(min(send_size, _stream.buffer_size()));
        //如果发送完了，则添加FIN标志
        //fin可以在包含负载的段中发送，但是如果窗口在添加负载之后已经满了，
        //那么不能在包含负载的段中设置fin=true，也不能在本次fill_window()时单独发送fin=true 负载=0的段
        //需要在下一次调用fill_window()时再发送这个发送fin=true 负载=0的段
       if (_stream.eof() && _recv_seqno+window_size > _next_seqno+tcp_segment.length_in_sequence_space()) {
           tcp_segment.header().fin = true;
           fin_ = true;
       }
       // 发送
       send_tcp_segment(tcp_segment);
   }
}

void TCPSender::send_tcp_segment(TCPSegment &tcp_segment) {
   // 设置序号
   tcp_segment.header().seqno = wrap(_next_seqno, _isn);
   // 放入队列中
   _segments_out.push(tcp_segment);
   _segments_wait.push(tcp_segment);
   // 本地保存发送的数据大小
   _next_seqno += tcp_segment.length_in_sequence_space();
   bytes_in_flight_ += tcp_segment.length_in_sequence_space();
   // 启动重传计时器
   if (!retransmissions_timer_running_) {
       retransmissions_timer_running_ = true;
       retransmissions_timer_ = 0;
   }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
   uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
   // 超出了窗口范围
   if (abs_ackno > _next_seqno) {
       return;
   }
   // 设置新的窗口大小
   if (abs_ackno >= _recv_seqno) {
       _recv_seqno = abs_ackno;
       receiver_window_size_ = window_size;
   }
   // 删除已经确认发送成功的段
   bool pop = false;
   while (!_segments_wait.empty()) {
       TCPSegment tcp_segment = _segments_wait.front();
       // 当前队列头的段还未发送成功
       if (abs_ackno <
           unwrap(tcp_segment.header().seqno, _isn, _next_seqno) + tcp_segment.length_in_sequence_space()) {
           break;
       }
       // 发送成功，要出队列，修改发送的数据大小
       _segments_wait.pop();
       bytes_in_flight_ -= tcp_segment.length_in_sequence_space();
       // 重置重传超时时间、重传次数与重传计时器
       retransmission_timeout_ = initial_retransmission_timeout_;
       consecutive_retransmissions_ = 0;
       retransmissions_timer_ = 0;
       pop = true;
   }
   // 如果有新的空间打开，则尝试填充之
   if (pop) {
       fill_window();
   }
   // 重传计时器是否启动取决于发送方是否有未完成的数据
   retransmissions_timer_running_ = !_segments_wait.empty();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
   // 重传计时器是否启动
   if (!retransmissions_timer_running_) {
       return;
   }
   // 记录时间
   retransmissions_timer_ += ms_since_last_tick;
   // 已超时
   if (retransmissions_timer_ >= retransmission_timeout_ && !_segments_wait.empty()) {
       // 重传数据
       TCPSegment tcp_segment = _segments_wait.front();
       _segments_out.push(tcp_segment);
       // 重置计时器
       retransmissions_timer_ = 0;
       // 记录连续重传的数量，并将RTO值翻倍
       if (receiver_window_size_ > 0 || tcp_segment.header().syn) {
           consecutive_retransmissions_++;
           retransmission_timeout_ *= 2;
       }
   }
}

unsigned int TCPSender::consecutive_retransmissions() const { return consecutive_retransmissions_; }

void TCPSender::send_empty_segment() {
   TCPSegment tcp_segment;
   tcp_segment.header().seqno = wrap(_next_seqno, _isn);
   _segments_out.push(tcp_segment);
}