#include "RingBuffer.h"

#include <memory>
#include <cstring>
#include <iostream>

namespace CasperTech
{
    RingBuffer::RingBuffer(size_t size)
            : _buf(std::make_unique<uint8_t[]>(size))
            , _maxSize(size)
    {

    }

    void RingBuffer::reset()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _head = _tail;
        _eos = -1;
        _shutdown = false;
        _full = false;
    }

    bool RingBuffer::empty() const
    {
        return (!_full && (_head == _tail));
    }

    bool RingBuffer::full() const
    {
        return _full;
    }

    void RingBuffer::shutdown()
    {
        _shutdown = true;
        _wait.notify_one();
    }

    size_t RingBuffer::capacity() const
    {
        return _maxSize;
    }

    size_t RingBuffer::size() const
    {
        size_t size = _maxSize;

        if(!_full)
        {
            if(_head >= _tail)
            {
                size = _head - _tail;
            }
            else
            {
                size = _maxSize + _head - _tail;
            }
        }

        return size;
    }

    void RingBuffer::put(const uint8_t* buf, size_t bytes)
    {
        _eos = -1;
        if (bytes == 0)
        {
            return;
        }
        std::unique_lock<std::mutex> lock(_mutex);
        size_t srcBufPos = 0;
        size_t destBufPos = _head;
        size_t bytesLeft = bytes;
        while(srcBufPos < bytes)
        {
            while(_full && !_shutdown)
            {
                _wait.wait(lock, [this]
                {
                    return !_full || _shutdown;
                });
            }
            if (_shutdown)
            {
                std::cout << "shutdown" << std::endl;
                return;
            }

            size_t space = _maxSize - size();
            size_t bytesToWrite = bytesLeft;
            if (bytesToWrite > space)
            {
                bytesToWrite = space;
            }

            if (destBufPos + bytesToWrite > _maxSize)
            {
                size_t bytesRemaining = _maxSize - destBufPos;
                memcpy(&_buf[destBufPos], buf + srcBufPos, bytesRemaining);
                destBufPos = 0;
                srcBufPos += bytesRemaining;
                bytesLeft -= bytesRemaining;
                bytesToWrite -= bytesRemaining;
            }
            memcpy(&_buf[destBufPos], buf + srcBufPos, bytesToWrite);
            destBufPos += bytesToWrite;
            if (destBufPos >= _maxSize)
            {
                destBufPos -= _maxSize;
            }
            srcBufPos += bytesToWrite;
            bytesLeft -= bytesToWrite;

            _head = destBufPos;
            _full = _head == _tail;
            _wait.notify_one();
        }
    }

    int RingBuffer::get(uint8_t* buf, size_t bytes)
    {
        if (bytes == 0)
        {
            return 0;
        }
        std::unique_lock<std::mutex> lock(_mutex);
        size_t srcBufPos = _tail;
        size_t destBufPos = 0;
        size_t bytesLeft = bytes;
        while(destBufPos < bytes)
        {
            while(size() == 0 && !_shutdown)
            {
                _wait.wait(lock, [this]
                {
                    return size() > 0 || _shutdown;
                });
            }
            if (_shutdown)
            {
                std::cout << "shutdown" << std::endl;
                return 1;
            }

            size_t bytesToWrite = size();
            if (bytesToWrite > bytesLeft)
            {
                bytesToWrite = bytesLeft;
            }
            if(_tail + bytesToWrite > _maxSize)
            {
                size_t remainingBytes = _maxSize - _tail;
                memcpy(&buf[destBufPos], &_buf[srcBufPos], remainingBytes);
                srcBufPos = 0;
                destBufPos += remainingBytes;
                bytesLeft -= remainingBytes;
                bytesToWrite -= remainingBytes;
            }
            memcpy(&buf[destBufPos], &_buf[srcBufPos], bytesToWrite);
            srcBufPos += bytesToWrite;
            if (srcBufPos >= _maxSize)
            {
                srcBufPos -= _maxSize;
            }
            destBufPos += bytesToWrite;
            bytesLeft -= bytesToWrite;
            _tail = srcBufPos;
            if (_eos > -1 && _eos == _tail)
            {
                _shutdown = true;
                _wait.notify_one();
                return 1;
            }
            _full = false;
            _wait.notify_one();
        }
        return 0;
    }

    void RingBuffer::eos()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _eos = static_cast<int64_t>(_head);
    }

    RingBuffer::~RingBuffer()
    {
        shutdown();
    }
}
