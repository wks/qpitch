#pragma once

#include <cstdlib>
#include <vector>

class CyclicBuffer
{
public:
    explicit CyclicBuffer(size_t capacity);
    // Append len bytes from src to the cyclic buffer.
    void append(const unsigned char *src, size_t len);
    // Copy the last len bytes to dst.  Return the actual number of bytes copied.
    size_t copyLastBytes(unsigned char *dst, size_t len) const;

private:
    // The capacity of _buffer.
    size_t _capacity;
    // The index of the current byte to write at.
    size_t _cursor;
    // true if we have filled _buffer with _capacity bytes already.
    // When true, all bytes in _buffer are written data; otherwise only bytes before _cursor are written.
    bool _filledOnce;
    // The buffer.
    std::vector<unsigned char> _buffer;
};
