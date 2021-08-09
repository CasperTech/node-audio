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
        shutdown();
        
        std::lock_guard<std::mutex> lock(_mutex);
        _head = _tail;
        _eos = -1;
        _shutdownGet = false;
        _shutdownPut = false;
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
        std::unique_lock<std::mutex> shutdownGetLock(_shutdownGetMutex);
        std::unique_lock<std::mutex> shutdownPutLock(_shutdownPutMutex);
        if (_runningPut)
        {
            _shutdownPut = true;
            _wait.notify_all();
            _shutdownPutWait.wait(shutdownPutLock);
        }
        if (_runningGet)
        {
            _shutdownGet = true;
            _wait.notify_all();
            _shutdownGetWait.wait(shutdownGetLock);
        }
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
        {
            std::unique_lock<std::mutex> lk(_shutdownPutMutex);
            _runningPut = true;
        }
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
            while(_full && !_shutdownPut)
            {
                _wait.wait(lock, [this]
                {
                    return !_full || _shutdownPut;
                });
            }
            if (_shutdownPut)
            {
#ifdef _DEBUG
                std::cout << "shutdown" << std::endl;
#endif
                break;
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
        std::unique_lock<std::mutex> lk(_shutdownPutMutex);
        _runningPut = false;
        if (_shutdownPut)
        {
            _shutdownPutWait.notify_all();
        }
    }

    int RingBuffer::get(uint8_t* buf, size_t bytes)
    {
        {
            std::unique_lock<std::mutex> lk(_shutdownGetMutex);
            _runningGet = true;
        }
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
            while(size() == 0 && !_shutdownGet)
            {
                _wait.wait(lock, [this]
                {
                    return size() > 0 || _shutdownGet;
                });
            }
            if (_shutdownGet)
            {
#ifdef _DEBUG
                std::cout << "shutdown" << std::endl;
#endif
                break;
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
                _shutdownGet = true;
                _wait.notify_one();
                return 1;
            }
            _full = false;
            _wait.notify_one();
        }
        std::unique_lock<std::mutex> lk(_shutdownGetMutex);
        _runningGet = false;
        if (_shutdownGet)
        {
            _shutdownGetWait.notify_all();
            return 1;
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
