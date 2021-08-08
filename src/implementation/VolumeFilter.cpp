#include <enums/AudioError.h>
#include <exceptions/AudioException.h>
#include "VolumeFilter.h"

#include "FFSource.h"

#include <cassert>
#include <iostream>

#define VOLUME_VAL 0.1

namespace CasperTech
{
    VolumeFilter::VolumeFilter()
    {
        
    }

    std::string VolumeFilter::getName() const
    {
        return "VolumeFilter";
    }

    int VolumeFilter::checkError(int errnum)
    {
        if (errnum < 0)
        {
            std::cout << "Err: " << FFSource::getError(errnum) << std::endl;
            throw AudioException(AudioError::PipelineError, FFSource::getError(errnum));
        }
        return errnum;
    }

    SampleFormatFlags VolumeFilter::getSupportedSampleFormats()
    {
        if (_source)
        {
            return _source->getSupportedSampleFormats();
        }
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

    std::vector<uint32_t> VolumeFilter::getSupportedSampleRates()
    {
        if (_source)
        {
            return _source->getSupportedSampleRates();
        }
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

    uint8_t VolumeFilter::getSupportedChannels()
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

    void VolumeFilter::audio(const uint8_t* buffer, uint64_t sampleCount)
    {
        std::unique_lock<std::mutex> lk(_pipelineMutex);
        _frame->nb_samples = static_cast<int>(sampleCount);
        _frame->sample_rate = _sourceSampleRate;
        _frame->format = _avSampleFormat;
        _frame->channel_layout = _avChannelLayout;
        _frame->pts = _pts;

        checkError(av_frame_get_buffer(_frame, 0));

       
        //av_samples_copy(&_frame->extended_data[0], const_cast<uint8_t*const*>(&buffer), 0, 0, static_cast<int>(sampleCount), _sourceChannels, _avSampleFormat);
        
        int bufSize = _sampleSize * _sourceChannels * static_cast<int>(sampleCount);
        memcpy_s(_frame->extended_data[0], bufSize, buffer, bufSize);
        
        _pts += _sourceSampleRate;

        float* tmpFloat = reinterpret_cast<float*>(_frame->extended_data[0]);

        checkError(av_buffersrc_add_frame(_aBufferCtx, _frame));
        int err = 0;
        while ((err = av_buffersink_get_frame(_aBufferSinkCtx, _frame)) >= 0)
        {
            float* newFloat = reinterpret_cast<float*>(_frame->extended_data[0]);
            if (_sink)
            {
                _sink->audio(_frame->extended_data[0], _frame->nb_samples);
            }
            av_frame_unref(_frame);
            checkError(err);
        }
    }

    void VolumeFilter::onSourceConfigured()
    {
        _sourceConfigured = true;
        if (_sinkConfigured)
        {
            // Re-connect sink to establish preferred sample rate and channels
            connectSink(_sink);
        }
    }

    void VolumeFilter::onSinkConfigured()
    {
        _sinkConfigured = true;
        if (_sourceConfigured)
        {
            init();
        }

        switch(_sourceFormat)
        {
            case SampleFormatFlags::S8:
            case SampleFormatFlags::S8_Planar:
                _sampleSize = 1;
                break;
            case SampleFormatFlags::S16:
            case SampleFormatFlags::S16_Planar:
                _sampleSize = 2;
                break;
            case SampleFormatFlags::S24:
            case SampleFormatFlags::S24_Planar:
                _sampleSize = 4;
                break;
            case SampleFormatFlags::S32:
            case SampleFormatFlags::S32_Planar:
                _sampleSize = 4;
                break;
            case SampleFormatFlags::FLT:
            case SampleFormatFlags::FLT_Planar:
                _sampleSize = 4;
                break;
            case SampleFormatFlags::DBL:
            case SampleFormatFlags::DBL_Planar:
                _sampleSize = 8;
                break;
            default:
                return;
        }
    }

    void VolumeFilter::onEos()
    {
        if (_sink)
        {
            _sink->onEos();
        }
    }

    void VolumeFilter::init()
    {
        if (_filterGraph != nullptr)
        {
            avfilter_graph_free(&_filterGraph);
            av_frame_free(&_frame);
        }

        _filterGraph = avfilter_graph_alloc();
        if (!_filterGraph)
        {
            throw AudioException(AudioError::PipelineError, "Unable to create filter graph");
        }

        _aBufferFilter = avfilter_get_by_name("abuffer");
        if (!_aBufferFilter)
        {
            throw AudioException(AudioError::PipelineError, "Unable to create aBuffer filter");
        }

        _aBufferCtx = avfilter_graph_alloc_filter(_filterGraph, _aBufferFilter, "src");
        if (!_aBufferCtx)
        {
            throw AudioException(AudioError::PipelineError, "Unable to create aBuffer filter context");
        }

        uint8_t ch_layout[64];
        _avChannelLayout = av_get_default_channel_layout(_sourceChannels);
        av_get_channel_layout_string(reinterpret_cast<char*>(ch_layout), sizeof(ch_layout), 0, _avChannelLayout);
        av_opt_set(_aBufferCtx, "channel_layout", reinterpret_cast<const char*>(ch_layout), AV_OPT_SEARCH_CHILDREN);
        _avSampleFormat = getAvSampleFormat(_sourceFormat);
        av_opt_set(_aBufferCtx, "sample_fmt", av_get_sample_fmt_name(_avSampleFormat),
                   AV_OPT_SEARCH_CHILDREN);
        av_opt_set_q(_aBufferCtx, "time_base", AVRational{1, static_cast<int>(_sourceSampleRate)},
                     AV_OPT_SEARCH_CHILDREN);
        av_opt_set_int(_aBufferCtx, "sample_rate", _sourceSampleRate, AV_OPT_SEARCH_CHILDREN);

        checkError(avfilter_init_str(_aBufferCtx, NULL));

        _frame = av_frame_alloc();
        if (!_frame)
        {
            throw AudioException(AudioError::PipelineError, "Unable to allocate frame");
        }

        _volumeFilter = avfilter_get_by_name("volume");
        if(!_volumeFilter)
        {
            throw AudioException(AudioError::PipelineError, "Unable to create volume filter");
        }

        _volumeCtx = avfilter_graph_alloc_filter(_filterGraph, _volumeFilter, "volume");
        if(!_volumeCtx)
        {
            throw AudioException(AudioError::PipelineError, "Unable to create abuffer filter context");
        }

        std::string vol = std::to_string(_volume);
        int err = av_opt_set(_volumeCtx, "volume", vol.c_str(), AV_OPT_SEARCH_CHILDREN);
        checkError(err);


        checkError(avfilter_init_str(_volumeCtx, NULL));

        _aBufferSink = avfilter_get_by_name("abuffersink");
        if (!_aBufferSink)
        {
            throw AudioException(AudioError::PipelineError, "Unable to create aBufferSink filter");
        }

        _aBufferSinkCtx = avfilter_graph_alloc_filter(_filterGraph, _aBufferSink, "sink");
        if (!_aBufferSinkCtx)
        {
            throw AudioException(AudioError::PipelineError, "Unable to create aBufferSink filter context");
        }

        checkError(avfilter_init_str(_aBufferSinkCtx, NULL));

        checkError(avfilter_link(_aBufferCtx, 0, _volumeCtx, 0));
        checkError(avfilter_link(_volumeCtx, 0, _aBufferSinkCtx, 0));

        avfilter_graph_config(_filterGraph, NULL);
    }

    AVSampleFormat VolumeFilter::getAvSampleFormat(SampleFormatFlags sampleFormat)
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

    VolumeFilter::~VolumeFilter()
    {
        if (_filterGraph != nullptr)
        {
            avfilter_graph_free(&_filterGraph);
            av_frame_free(&_frame);
        }
    }

    void VolumeFilter::setVolume(float volume)
    {
        std::unique_lock<std::mutex> lk(_pipelineMutex);
        _volume = volume;

        std::string vol = std::to_string(_volume);
        checkError(avfilter_graph_send_command(_filterGraph, "volume", "volume", vol.c_str(), NULL, 0, 0));
    }
}
