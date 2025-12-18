#include "cyclicbuffer.h"

#include <cstring>
#include <QtAssert>

CyclicBuffer::CyclicBuffer(size_t capacity) : _capacity(capacity)
{
    _cursor = 0;
    _filledOnce = false;
    _buffer.resize(capacity);
}

void CyclicBuffer::append(const unsigned char *src, size_t len)
{
    if (len >= _capacity) {
        // Too many bytes to append.  Just fill the entire buffer.
        size_t srcOffset = len - _capacity;
        memcpy(_buffer.data(), src + srcOffset, _capacity);
        _cursor = 0;
        _filledOnce = true;
    } else {
        size_t rightLen = _capacity - _cursor;
        if (len < rightLen) {
            // Not enough to fill the buffer.  Fill whatever src has.
            memcpy(&_buffer[_cursor], src, len);
            _cursor += len;
        } else {
            // Will fill the buffer to the right end.
            memcpy(&_buffer[_cursor], src, rightLen);
            _filledOnce = true;

            // Fill the remaining at the beginning.
            size_t remaining = len - rightLen;
            memcpy(_buffer.data(), src + rightLen, remaining);
            _cursor = remaining;
        }
    }
}

size_t CyclicBuffer::copyLastBytes(unsigned char *dst, size_t len) const
{
    size_t actualBytes = _filledOnce ? _capacity : _cursor;
    size_t truncatedLen = std::min(len, actualBytes);

    if (truncatedLen <= _cursor) {
        memcpy(dst, &_buffer[_cursor - truncatedLen], truncatedLen);
    } else {
        // If truncatedLen > _cursor, we must have more than _cursor bytes.
        Q_ASSERT(_filledOnce);
        size_t rightLen = truncatedLen - _cursor;
        size_t rightStart = _capacity - rightLen;
        memcpy(dst, &_buffer[rightStart], rightLen);

        memcpy(dst + rightLen, _buffer.data(), _cursor);
    }

    return truncatedLen;
}
