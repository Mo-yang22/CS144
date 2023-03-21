#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),_buf(capacity,'\0'),_flag(capacity,false) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t first_unassembled=_output.bytes_written();
    size_t first_unacceptable=first_unassembled+_capacity;
    if(index>=first_unacceptable || index+data.size()<first_unassembled)
        return;
    size_t begin_index=index;
    size_t end_index=index+data.size();
    if(begin_index<first_unassembled)
        begin_index=first_unassembled;
    if(end_index>first_unacceptable)
        end_index=first_unacceptable;
    for(size_t i=begin_index;i<end_index;++i){
        if(!_flag[(i)%_capacity]){
            _buf[(i)%_capacity]=data[i-index];
            _flag[(i)%_capacity]=true;
            unass_bytes++;
        }
    }
    string tmp;
    while(_flag[_index%_capacity]){
        tmp+=_buf[_index%_capacity];
        _flag[_index%_capacity]=false;
        _index++;
        unass_bytes--;
    }
    if(tmp.size()>0)
        _output.write(tmp);
    if(eof)
    {
        _eof=eof;
        _eof_index=end_index;
    }
    if(_eof&&_eof_index==_output.bytes_written())
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return unass_bytes; }

bool StreamReassembler::empty() const { return (unassembled_bytes()==0); }
