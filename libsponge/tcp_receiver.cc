#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
   
    if(seg.header().syn==true&&_syn)
    {
        _reassembler.stream_out().set_error();
        return;
    }
    //第一个到达的，设置了SYN标志的段的序列号是初始序列号
    if(seg.header().syn)
    {
        //在这里发现了一个bug
        //我之前是没有设置一个syn位的,
        //仅用了isn=wrap(0,seg.header().seqno);
        //但事实证明不行,因为seqno可能为0,导致出现了bug
        isn=seg.header().seqno;
        _syn=true;
        // return;
        //可能标志位既有S，又有F
    }
    
    //string_view的成员函数即对外接口与 string 相类似，但只包含读取字符串内容的部分。
    //下一步是push_substring函数的使用

    //得到index,checkpoint是最后一个重新组装的索引(是指绝对序号)
    uint64_t index=unwrap(seg.header().seqno,isn,_reassembler.stream_out().bytes_written()+1)-1;
    if(seg.header().syn)
        index=0;
    _reassembler.push_substring(seg.payload().copy(),index,seg.header().fin);

}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!_syn)
        return nullopt;
    size_t stream_index=_reassembler.stream_out().bytes_written();
    if(_reassembler.stream_out().input_ended())
        ++stream_index;
    return wrap(static_cast<uint64_t>(stream_index+1),isn);
 }

size_t TCPReceiver::window_size() const { return _capacity-_reassembler.stream_out().buffer_size(); }//注意喂装配的部分其实算是window_size里面
