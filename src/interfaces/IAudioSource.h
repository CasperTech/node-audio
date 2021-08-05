#pragma once

#include "IAudioNode.h"

#include <memory>
#include <mutex>
#include <vector>

namespace CasperTech
{
    class IAudioSink;
    class IAudioSource: public IAudioNode, public std::enable_shared_from_this<IAudioSource>
    {
        public:
            virtual ~IAudioSource() = default;
            virtual void connectSink(const std::shared_ptr<IAudioSink>& sink);
            virtual void onSinkConfigured(){};
            virtual void eos();

            static std::vector<SampleFormatFlags> preferredFormatsAscending;

        protected:
            std::mutex _sinkMutex;
            std::shared_ptr<IAudioSink> _sink;

            SampleFormatFlags _sinkFormat = SampleFormatFlags::None;
            uint32_t _sinkSampleRate = 0;
            uint8_t _sinkChannels = 0;
    };
}

