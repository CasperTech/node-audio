#include "IAudioSource.h"

#include <interfaces/IAudioSink.h>
#include <exceptions/AudioException.h>

#include <algorithm>
#ifdef _DEBUG
#include <iostream>
#endif

namespace CasperTech
{
    std::vector<SampleFormatFlags> IAudioSource::preferredFormatsAscending = {
            SampleFormatFlags::DBL,
            SampleFormatFlags::DBL_Planar,
            SampleFormatFlags::FLT,
            SampleFormatFlags::FLT_Planar,
            SampleFormatFlags::S32,
            SampleFormatFlags::S32_Planar,
            SampleFormatFlags::S16,
            SampleFormatFlags::S16_Planar,
            SampleFormatFlags::U8,
            SampleFormatFlags::U8_Planar
    };

    void IAudioSource::connectSink(const std::shared_ptr<IAudioSink>& sink)
    {
        SampleFormatFlags selectedFormat = SampleFormatFlags::None;
        SampleFormatFlags source = getSupportedSampleFormats();
        SampleFormatFlags dest = sink->getSupportedSampleFormats();
        for(SampleFormatFlags flag: preferredFormatsAscending)
        {
            if ((source & flag) != 0 && (dest & flag) != 0)
            {
                selectedFormat = flag;
                break;
            }
        }

        if (selectedFormat == SampleFormatFlags::None)
        {
            throw AudioException(AudioError::FormatMismatch, "Could not agree on a suitable sample format");
        }

        uint32_t sampleRate = 0;
        std::vector<uint32_t> sourceRates = getSupportedSampleRates();
        std::vector<uint32_t> sinkRates = sink->getSupportedSampleRates();

        //sort(sourceRates.begin(), sourceRates.end(), std::greater<>());

        for(const uint32_t sourceRate: sourceRates)
        {
            for(const uint32_t sinkRate: sinkRates)
            {
                if (sourceRate == sinkRate)
                {
                    sampleRate = sourceRate;
                    break;
                }
            }
            if (sampleRate != 0)
            {
                break;
            }
        }

        if (sampleRate == 0)
        {
            throw AudioException(AudioError::FormatMismatch, "Could not agree on a suitable sample rate");
        }

        std::unique_lock<std::mutex> sinkLock(_sinkMutex);
        _sinkFormat = selectedFormat;
        _sinkSampleRate = sampleRate;
        _sinkChannels = getSupportedChannels();
        if(_sinkChannels == 0)
        {
            _sinkChannels = sink->getSupportedChannels();
        }
        _sink = sink;
        _sink->setSource(shared_from_this(), selectedFormat, sampleRate, _sinkChannels);

#ifdef _DEBUG
        std::cout << getName() << " -> " << sink->getName() << ": " << selectedFormat << ", sameplerate: " << sampleRate << ", channels: " << unsigned(_sinkChannels) << std::endl;
#endif

        onSinkConfigured();
    }

    void IAudioSource::eos()
    {
        if (_sink)
        {
            _sink->onEos();
        }
    }
}
