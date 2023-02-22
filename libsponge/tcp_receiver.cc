#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();
    if (!this->_syn) {
        if (!header.syn)
            return;
        this->_syn = true;
        this->_isn = header.seqno;
    }

    uint64_t checkpoint = this->_reassembler.stream_out().bytes_written();
    uint64_t index = unwrap(header.seqno, this->_isn, checkpoint) - 1 + (header.syn);
    this->_reassembler.push_substring(seg.payload().copy(), index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!this->_syn)
        return std::nullopt;
    uint64_t abs_ackno = this->_reassembler.stream_out().bytes_written() + 1 + this->_reassembler.stream_out().input_ended();
    return wrap(abs_ackno, this->_isn);
}

size_t TCPReceiver::window_size() const {
    return this->_capacity - this->_reassembler.stream_out().buffer_size();
}
