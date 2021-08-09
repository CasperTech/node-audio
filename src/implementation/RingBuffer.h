#include <condition_variable>
#include <memory>
#include <mutex>

namespace CasperTech
{
    class RingBuffer
    {
        public:
            explicit RingBuffer(size_t size);
            ~RingBuffer();

            void put(const uint8_t* buf, size_t size);

            int get(uint8_t* buf, size_t size);

            void reset();

            void shutdown();

            void eos();

            [[nodiscard]] bool empty() const;

            [[nodiscard]] bool full() const;

            [[nodiscard]] size_t capacity() const;

            [[nodiscard]] size_t size() const;

        private:
            std::mutex _mutex;
            std::mutex _shutdownPutMutex;
            std::mutex _shutdownGetMutex;
            std::condition_variable _wait;
            std::condition_variable _shutdownPutWait;
            std::condition_variable _shutdownGetWait;
            std::unique_ptr<uint8_t[]> _buf;
            size_t _head = 0;
            size_t _tail = 0;
            int64_t _eos = -1;
            const size_t _maxSize;
            bool _full = false;
            bool _shutdownPut = false;
            bool _runningPut = false;
            bool _shutdownGet = false;
            bool _runningGet = false;
    };

}

