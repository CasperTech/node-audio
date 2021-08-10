#include <interfaces/IAudioSink.h>

#include <iostream>

namespace CasperTech
{
    void IAudioSink::setSource(const std::shared_ptr<IAudioSource>& source, SampleFormatFlags fmt,
                                           uint32_t sampleRate, uint8_t channels)
    {
        _source = source;
        _sourceFormat = fmt;
        _sourceSampleRate = sampleRate;
        _sourceChannels = channels;

        onSourceConfigured();
    }

    void IAudioSink::disconnectSource()
    {
        _source.reset();
    }
}
