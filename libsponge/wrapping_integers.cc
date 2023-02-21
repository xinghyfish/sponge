#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint32_t step = n & 0xffffffff;
    return isn + step;
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint64_t ref = ((checkpoint >> 32) << 32);
    uint32_t bias = n - isn;
    if (checkpoint == 0ul) {
        return static_cast<uint64_t>(bias);
    } else if (ref + bias > checkpoint) {
        if (ref + bias - checkpoint > checkpoint + (1ul << 32) - (ref + bias))
            return ref + bias - (1ul << 32);
        else
            return ref + bias;
    } else {
        if (ref + bias + (1ul << 32) - checkpoint > checkpoint - (ref + bias))
            return ref + bias;
        else 
            return ref + bias + (1ul << 32);
    }
}
