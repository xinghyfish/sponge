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
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
    return _outstanding_byte_counter;
}

void TCPSender::fill_window() {
    // window size = 0, treat as 1
    uint16_t window_size = max(_window_size, static_cast<uint16_t> (1));
    while (window_size > _outstanding_byte_counter) {
        TCPSegment seg;
        if (!_syn) {
            seg.header().syn = true;
            _syn = true;
        }
        seg.header().seqno = next_seqno();
        size_t payload_size = min(TCPConfig::MAX_PAYLOAD_SIZE, window_size - _outstanding_byte_counter - seg.header().syn);
        seg.payload() = Buffer(std::move(_stream.read(payload_size)));

        if (!_fin && _stream.eof() && seg.payload().size() + _outstanding_byte_counter < window_size)
            _fin = seg.header().fin = true;

        if (seg.length_in_sequence_space() == 0)
            break;
        
        // SYN segment, start timer
        if (_outstanding_segments.empty()) {
            _timeout = _initial_retransmission_timeout;
            _timer = 0;
        }
        // send segment
        _segments_out.push(seg);
        // keep track of the segment
        _outstanding_byte_counter += seg.length_in_sequence_space();
        _outstanding_segments.insert({_next_seqno, seg});
        // update next seqno absolute
        _next_seqno += seg.length_in_sequence_space();

        if (seg.header().fin)
            break;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
    // abort unreliable segment
    if (abs_ackno > _next_seqno)
        return;
    for (auto it = _outstanding_segments.begin(); it != _outstanding_segments.end(); ) {
        if (abs_ackno >= it->first + it->second.length_in_sequence_space()) {
            _outstanding_byte_counter -= it->second.length_in_sequence_space();
            it = _outstanding_segments.erase(it);

            _timeout = _initial_retransmission_timeout;
            _timer = 0;
        } else {
            break;
        }
    }

    _consecutive_retransmissions_counter = 0;
    _window_size = window_size;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer += ms_since_last_tick;

    // timer expired
    if (_timer >= _timeout && !_outstanding_segments.empty()) {
        _segments_out.push(_outstanding_segments.begin()->second);
        if (_window_size) {
            _timeout <<= 1;
        }
        _consecutive_retransmissions_counter++;
        _timer  = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _consecutive_retransmissions_counter;
}

void TCPSender::send_empty_segment() {
    TCPSegment empty_seg;
    empty_seg.header().seqno = _isn;
    _segments_out.push(empty_seg);
}
