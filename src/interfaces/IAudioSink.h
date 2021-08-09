#pragma once

#include "IAudioNode.h"

namespace CasperTech
{
    class IAudioSource;
    class IAudioSink: public IAudioNode
    {
        public:
            virtual ~IAudioSink() = default;
            virtual void setSource(const std::shared_ptr<IAudioSource>& source, SampleFormatFlags fmt, uint32_t sampleRate, uint8_t channels);
            virtual void audio(const uint8_t* buffer, const uint8_t* planarChannel, uint64_t sampleCount) = 0;
            virtual void onSourceConfigured(){}
            virtual void onEos() = 0;

        protected:
            std::shared_ptr<IAudioSource> _source;
            SampleFormatFlags _sourceFormat = SampleFormatFlags::None;
            uint32_t _sourceSampleRate = 0;
            uint8_t _sourceChannels = 0;

    };
}