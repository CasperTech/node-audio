#include <enums/AudioError.h>
#include <exceptions/AudioException.h>
#include "SampleRateConverter.h"
#include "FFSource.h"

namespace CasperTech
{
    SampleFormatFlags SampleRateConverter::getSupportedSampleFormats()
    {
        return SampleFormatFlags::U8_Planar
               | SampleFormatFlags::U8
               | SampleFormatFlags::S16_Planar
               | SampleFormatFlags::S16
               | SampleFormatFlags::S32
               | SampleFormatFlags::S32_Planar
               | SampleFormatFlags::FLT
               | SampleFormatFlags::FLT_Planar
               | SampleFormatFlags::DBL
               | SampleFormatFlags::DBL_Planar;
    }

    std::vector<uint32_t> SampleRateConverter::getSupportedSampleRates()
    {
        std::vector<uint32_t> supported = std::vector<uint32_t>{
                384000,
                352800,
                192000,
                176400,
                96000,
                88200,
                48000,
                44100,
                32000,
                22050,
                11025,
                8000
        };;
        if (_source)
        {
            supported = _source->getSupportedSampleRates();
        }
        return supported;
    }

    uint8_t SampleRateConverter::getSupportedChannels()
    {
        if (_source)
        {
            return _sourceChannels;
        }
        if (_sink)
        {
            return _sinkChannels;
        }
        return 2;
    }

    void SampleRateConverter::audio(void* buffer, uint64_t sampleCount)
    {
        //std::cout << "Audio " << sampleCount << " samples" << std::endl;
        if(!_configured)
        {
            throw AudioException(AudioError::PipelineError, "Sink or source not yet set");
        }
        auto dst_nb_samples = av_rescale_rnd(swr_get_delay(_swrCtx, _sourceSampleRate) + sampleCount, _sinkSampleRate, _sourceSampleRate, AV_ROUND_UP);
        if (dst_nb_samples > _maxDstSamples)
        {
            if (_maxDstSamples != 0)
            {
                av_free(&_dstData[0]);
            }
            checkError(av_samples_alloc(reinterpret_cast<uint8_t**>(&_dstData), &_dstLineSize, _sinkChannels, static_cast<int>(dst_nb_samples), _destFormat, 1));
            _maxDstSamples = dst_nb_samples;
        }
        int samplesConverted = checkError(swr_convert(_swrCtx, reinterpret_cast<uint8_t**>(&_dstData), static_cast<int>(dst_nb_samples), reinterpret_cast<const uint8_t**>(buffer), static_cast<int>(sampleCount)));
        if (_sink)
        {
            _sink->audio(_dstData, samplesConverted);
        }
    }

    SampleRateConverter::SampleRateConverter()
    {
        _swrCtx = swr_alloc();
    }

    SampleRateConverter::~SampleRateConverter()
    {
        if (_dstData)
        {
            av_freep(&_dstData[0]);
        }
        av_freep(&_dstData);

        swr_free(&_swrCtx);
        _swrCtx = nullptr;
    }

    void SampleRateConverter::init()
    {
        _srcFormat = getSwrSampleFormat(_sourceFormat);
        _destFormat = getSwrSampleFormat(_sinkFormat);

        // std::cout << "Converting from " << _sourceFormat << " " << _sourceSampleRate << " to " << _sinkFormat << " " << _sinkSampleRate << std::endl;

        auto srcLayout = av_get_default_channel_layout(_sourceChannels);
        av_opt_set_int(_swrCtx, "in_channel_layout", srcLayout, 0);
        av_opt_set_int(_swrCtx, "in_sample_rate", _sourceSampleRate, 0);
        av_opt_set_sample_fmt(_swrCtx, "in_sample_fmt", _srcFormat, 0);

        av_opt_set_int(_swrCtx, "out_channel_layout", av_get_default_channel_layout(_sinkChannels), 0);
        av_opt_set_int(_swrCtx, "out_sample_rate", _sinkSampleRate, 0);
        av_opt_set_sample_fmt(_swrCtx, "out_sample_fmt", _destFormat, 0);

        checkError(swr_init(_swrCtx));
        _configured = true;
        // std::cout << "SwrConfigured" << std::endl;
    }

    AVSampleFormat SampleRateConverter::getSwrSampleFormat(SampleFormatFlags sampleFormat)
    {
        AVSampleFormat fmt = AV_SAMPLE_FMT_U8;
        if(sampleFormat == SampleFormatFlags::U8)
        {
            fmt = AV_SAMPLE_FMT_U8;
        }
        else if(sampleFormat == SampleFormatFlags::U8_Planar)
        {
            fmt = AV_SAMPLE_FMT_U8P;
        }
        else if(sampleFormat == SampleFormatFlags::S16)
        {
            fmt = AV_SAMPLE_FMT_S16;
        }
        else if(sampleFormat == SampleFormatFlags::S16_Planar)
        {
            fmt = AV_SAMPLE_FMT_S16P;
        }
        else if(sampleFormat == SampleFormatFlags::S32)
        {
            fmt = AV_SAMPLE_FMT_S32;
        }
        else if(sampleFormat == SampleFormatFlags::S32_Planar)
        {
            fmt = AV_SAMPLE_FMT_S32P;
        }
        else if(sampleFormat == SampleFormatFlags::FLT)
        {
            fmt = AV_SAMPLE_FMT_FLT;
        }
        else if(sampleFormat == SampleFormatFlags::FLT_Planar)
        {
            fmt = AV_SAMPLE_FMT_FLTP;
        }
        else if(sampleFormat == SampleFormatFlags::DBL)
        {
            fmt = AV_SAMPLE_FMT_DBL;
        }
        else if(sampleFormat == SampleFormatFlags::DBL_Planar)
        {
            fmt = AV_SAMPLE_FMT_DBLP;
        }
        return fmt;
    }

    int SampleRateConverter::checkError(int errnum)
    {
        if (errnum < 0)
        {
            std::cout << "Err: " << FFSource::getError(errnum) << std::endl;
            throw AudioException(AudioError::PipelineError, FFSource::getError(errnum));
        }
        return errnum;
    }

    void SampleRateConverter::onSourceConfigured()
    {
        _sourceConfigured = true;
        if (_sinkConfigured)
        {
            // Re-connect sink to establish preferred sample rate and channels
            connectSink(_sink);
        }
    }

    void SampleRateConverter::onSinkConfigured()
    {
        _sinkConfigured = true;
        if (_sourceConfigured)
        {
            init();
        }
    }

    void SampleRateConverter::onEos()
    {
        if (_sink)
        {
            _sink->onEos();
        }
    }
}
