#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _unassembled_map(), _expect_next_index(0), _eof_index(-1), _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    size_t i;
    size_t unassebled_rest = this->_capacity - this->_output.buffer_size() - this->_unassembled_map.size();
    size_t buffer_rest = this->_capacity - this->_output.buffer_size();
    if (index > this->_expect_next_index) {
        for (i = 0; i < data.length() && unassebled_rest; ++i) {
            if (this->_unassembled_map.find(index + i) == this->_unassembled_map.end())
                unassebled_rest--;
            this->_unassembled_map.insert({index + i, data[i]});
        }
    } else if (index + data.length() > this->_expect_next_index){
        // 未写入部分的字符串偏移量
        size_t start = this->_expect_next_index - index;
        for (i = start; i < data.length() && buffer_rest; ++i) {
            // 判断当前字节是否在Reassembler中
            if (this->_unassembled_map.find(index + i) != this->_unassembled_map.end()) {
                // 已经在reassembler中，直接写入
                this->_unassembled_map.erase(index + i);
            } else {
                buffer_rest--;
                if (!unassebled_rest)
                    this->_unassembled_map.erase(this->_unassembled_map.rbegin()->first);
                else
                    unassebled_rest--;
            }
        }
        this->_output.write(data.substr(start, i - start));
        this->_expect_next_index = index + i;

        std::string appenStr("");
        while (this->_unassembled_map.find(this->_expect_next_index) != this->_unassembled_map.end())
        {
            appenStr += this->_unassembled_map[this->_expect_next_index];
            this->_unassembled_map.erase(this->_expect_next_index);
            this->_expect_next_index++;
        }
        this->_output.write(appenStr);
    }

    if (eof) {
        this->_eof_index = index + data.length();
    }
    if (this->_expect_next_index == this->_eof_index)
        this->_output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { 
    return this->_unassembled_map.size();
}

bool StreamReassembler::empty() const {
    return this->_unassembled_map.empty();
}
