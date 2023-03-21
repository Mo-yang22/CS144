#include "byte_stream.hh"
#include <iostream>
#include <algorithm>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    maxsize=capacity;
 }

size_t ByteStream::write(const string &data) {
    if(input_ended())
    {
        set_error();
        std::cout<<"llllllllllllllllllllllll"<<endl;
        return 0;
    }
        

    // for(auto i:data)
    //     dq.push_back(i);
    // size_t s=data.size();
    // writenum+=s;
    // return s;
    size_t s=data.size();
    s=min(s,remaining_capacity());
    for(size_t i=0;i<s;++i)
    {
        dq.push_back(data[i]);
    }
    writenum+=s;
    return s;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string res;
    size_t s=dq.size();
    s=min(s,len);
    for(size_t i=0;i<s;++i)
    {
        char tmp=dq.at(i);
        res+=tmp;
    }
    return res;

}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t s=dq.size();
    s=min(s,len);
    for(size_t i=0;i<s;++i)
    {
        dq.pop_front();
    }
    readnum+=s;
 }

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string res=peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() {
    _finish=true;
}

bool ByteStream::input_ended() const { 
    return _finish;
 }

size_t ByteStream::buffer_size() const { 
    return dq.size();
 }

bool ByteStream::buffer_empty() const { return dq.empty(); }

bool ByteStream::eof() const { 
    return input_ended()&&buffer_empty();
 }

size_t ByteStream::bytes_written() const { 
    return writenum;
 }

size_t ByteStream::bytes_read() const { 
    return readnum;
 }

size_t ByteStream::remaining_capacity() const { 
    return maxsize-dq.size();
 }
