#include "FFSource.h"
#include "ScopedPacketUnref.h"

#include <implementation/FFFrame.h>
#include <interfaces/IAudioSink.h>
#include <exceptions/CommandException.h>
#include <exceptions/PlayerException.h>

#include <iostream>
#include <cassert>

namespace CasperTech
{
    void CasperTech::FFSource::load(const std::string& fileName)
    {
        checkError(avformat_open_input(&_fmtCtx, fileName.c_str(), nullptr, nullptr));
        checkError(avformat_find_stream_info(_fmtCtx, nullptr));

        for (uint32_t i = 0; i < _fmtCtx->nb_streams; i++)
        {
            AVStream* pStream = _fmtCtx->streams[i];
            if(pStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                _streamIndex = i;

                _audioCodec = pStream->codecpar;
                AVCodec* dec = avcodec_find_decoder(_audioCodec->codec_id);
                if (dec == nullptr)
                {
                    throw CommandException(CommandResult::LoadError, "No codec found to handle file");
                }
                _audioCtx = avcodec_alloc_context3(nullptr);
                checkError(avcodec_parameters_to_context(_audioCtx, _audioCodec));
                checkError(avcodec_open2(_audioCtx, dec, nullptr));
                _channels = _audioCodec->channels;
                _sampleRate = _audioCodec->sample_rate;

                if (_packetInitialised && _pkt.data != nullptr)
                {
                    av_packet_unref(&_pkt);
                }
                else
                {
                    av_init_packet(&_pkt);
                    _packetInitialised = true;
                }
                _stream = pStream;
                return;
            }
        }
        throw CommandException(CommandResult::LoadError, "No audio stream in file");
    }

    std::string CasperTech::FFSource::getError(int errnum)
    {
        static char cstr[AV_ERROR_MAX_STRING_SIZE];
        memset(cstr, 0, sizeof(cstr));
        av_make_error_string(cstr, AV_ERROR_MAX_STRING_SIZE, errnum);
        return std::string(cstr);
    }

    void FFSource::checkError(int errnum)
    {
        if (errnum < 0)
        {
            throw CommandException(CommandResult::LoadError, FFSource::getError(errnum));
        }
    }

    int FFSource::getPacket(FFFrame* frame)
    {
        if (_fmtCtx == nullptr)
        {
            std::cout << "fmtCtx -1" << std::endl;
            return -1;
        }

        int ret = av_read_frame(_fmtCtx, &_pkt);
        if (ret == AVERROR_EOF)
        {
            avformat_seek_file(_fmtCtx, _streamIndex, 0, 0,  0, AVSEEK_FLAG_BYTE);
            return 0;
        }
        if (ret < 0)
        {
            return ret;
        }
        ScopedPacketUnref pktScope(&_pkt);
        if (_pkt.stream_index == _streamIndex)
        {
            ret = avcodec_send_packet(_audioCtx, &_pkt);
            if (ret == -11)
            {
                // This state means we need to receive frames in the buffer
                // but really it shouldn't happen
                assert(false);
                ret = 1;
            }
            if (ret < 0)
            {
                return ret;
            }
            while(ret >= 0)
            {
                ret = avcodec_receive_frame(_audioCtx, frame->frame);
                if (ret == AVERROR_EOF)
                {
                    avformat_seek_file(_fmtCtx, _streamIndex, 0, 0, 0, AVSEEK_FLAG_BYTE);
                    return 0;
                }
                if (ret < 0)
                {
                    return ret;
                }

                // PROCESS AUDIO
                if(_sink)
                {
                    // std::cout << "Sending audio" << std::endl;
                    _sink->audio(frame->frame->extended_data[0], frame->frame->extended_data[1], frame->frame->nb_samples);
                }
                else
                {
                    std::cout << "No sink" << std::endl;
                }

                av_frame_unref(frame->frame);
            }
        }
        return 1;
    }

    SampleFormatFlags FFSource::getSupportedSampleFormats()
    {
        auto flags = SampleFormatFlags::None;
        if (_audioCodec == nullptr)
        {
            return flags;
        }

        if (_audioCodec->format == AV_SAMPLE_FMT_U8)
        {
            return SampleFormatFlags::U8;
        }
        else if (_audioCodec->format == AV_SAMPLE_FMT_S16)
        {
            return SampleFormatFlags::S16;
        }
        else if (_audioCodec->format == AV_SAMPLE_FMT_S32)
        {
            return SampleFormatFlags::S32;
        }
        else if (_audioCodec->format == AV_SAMPLE_FMT_FLT)
        {
            return SampleFormatFlags::FLT;
        }
        else if (_audioCodec->format == AV_SAMPLE_FMT_DBL)
        {
            return SampleFormatFlags::DBL;
        }
        else if (_audioCodec->format == AV_SAMPLE_FMT_U8P)
        {
            return SampleFormatFlags::U8_Planar;
        }
        else if (_audioCodec->format == AV_SAMPLE_FMT_S16P)
        {
            return SampleFormatFlags::S16_Planar;
        }
        else if (_audioCodec->format == AV_SAMPLE_FMT_S32P)
        {
            return SampleFormatFlags::S32_Planar;
        }
        else if (_audioCodec->format == AV_SAMPLE_FMT_FLTP)
        {
            return SampleFormatFlags::FLT_Planar;
        }
        else if (_audioCodec->format == AV_SAMPLE_FMT_DBLP)
        {
            return SampleFormatFlags::DBL_Planar;
        }
        return flags;
    }

    std::vector<uint32_t> FFSource::getSupportedSampleRates()
    {
        if (_audioCodec == nullptr)
        {
            return { 0 };
        }
        return { static_cast<uint32_t>(_audioCodec->sample_rate) };
    }

    uint8_t FFSource::getSupportedChannels()
    {
        if (_audioCodec == nullptr)
        {
            return 0;
        }
        return _audioCodec->channels;
    }

    FFSource::FFSource()
    {
        av_log_set_level(AV_LOG_QUIET);
    }

    FFSource::~FFSource() noexcept
    {
        if (_packetInitialised)
        {
            av_packet_unref(&_pkt);
        }
        if (_audioCtx != nullptr)
        {
            avcodec_close(_audioCtx);
            _audioCtx = nullptr;
        }
        if (_fmtCtx != nullptr)
        {
            avformat_close_input(&_fmtCtx);
            _fmtCtx = nullptr;
        }

    }

    void FFSource::seek(uint64_t timeMs)
    {
        if (!_stream || !_fmtCtx)
        {
            return;
        }
        double timeBase = av_q2d(_stream->time_base);

        auto ts = static_cast<int64_t>(timeMs / (timeBase * 1000));

        avformat_seek_file(_fmtCtx, _streamIndex, ts, ts,  ts, 0);
    }

    std::string FFSource::getName() const
    {
        return "FFSource";
    }
}

