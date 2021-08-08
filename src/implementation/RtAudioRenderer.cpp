#include "RtAudioRenderer.h"
#include "RtAudioStream.h"

#include <RtAudio.h>

namespace CasperTech
{
    RtAudioRenderer::RtAudioRenderer()
        : _currentStream(std::make_unique<RtAudioStream>())
    {

    }

    std::string RtAudioRenderer::getName() const
    {
        return "RtAudioRenderer";
    }

    std::map<uint32_t, std::string> RtAudioRenderer::getDevices()
    {
        std::shared_lock<std::shared_mutex> lk(_streamMutex);
        return _currentStream->getDevices();
    }

    SampleFormatFlags RtAudioRenderer::getSupportedSampleFormats()
    {
        std::shared_lock<std::shared_mutex> lk(_streamMutex);
        return _currentStream->getSupportedSampleFormats();
    }

    std::vector<uint32_t> RtAudioRenderer::getSupportedSampleRates()
    {
        std::shared_lock<std::shared_mutex> lk(_streamMutex);
        return _currentStream->getSupportedSampleRates();
    }

    uint8_t RtAudioRenderer::getSupportedChannels()
    {
        std::shared_lock<std::shared_mutex> lk(_streamMutex);
        return _currentStream->getSupportedChannels();
    }

    void RtAudioRenderer::audio(const uint8_t* buffer, uint64_t sampleCount)
    {
        std::shared_lock<std::shared_mutex> lk(_streamMutex);
        return _currentStream->audio(buffer, sampleCount);
    }

    void RtAudioRenderer::onSourceConfigured()
    {
        std::unique_lock<std::shared_mutex> lk(_streamMutex);
        _currentStream = std::make_unique<RtAudioStream>();
        if (_selectedDevice != -1)
        {
            _currentStream->selectDevice(_selectedDevice);
        }

        RtAudioFormat fmt;
        switch(_sourceFormat)
        {
            case SampleFormatFlags::S8:
                fmt = RTAUDIO_SINT8;
                _sampleSize = 1;
                break;
            case SampleFormatFlags::S16:
                fmt = RTAUDIO_SINT16;
                _sampleSize = 2;
                break;
            case SampleFormatFlags::S24:
                fmt = RTAUDIO_SINT24;
                _sampleSize = 4;
                break;
            case SampleFormatFlags::S32:
                fmt = RTAUDIO_SINT32;
                _sampleSize = 4;
                break;
            case SampleFormatFlags::FLT:
                fmt = RTAUDIO_FLOAT32;
                _sampleSize = 4;
                break;
            case SampleFormatFlags::DBL:
                fmt = RTAUDIO_FLOAT64;
                _sampleSize = 8;
                break;
            default:
                return;
        }

        auto frameSize = _sampleSize * _sourceChannels;
        auto bufFrames = static_cast<uint32_t>(((static_cast<float>(_sourceSampleRate) / 1000.0) * static_cast<float>(_bufferLengthMs)) / 2.0);
        auto bufSize = bufFrames * frameSize;

        _currentStream->configure(fmt, _sourceChannels, _sourceSampleRate, _sampleSize, bufFrames, bufSize);
    }


    void RtAudioRenderer::onEos()
    {
        std::shared_lock<std::shared_mutex> lk(_streamMutex);
        _currentStream->onEos();
    }

    void RtAudioRenderer::selectDevice(uint32_t device)
    {
        std::shared_lock<std::shared_mutex> lk(_streamMutex);
        _selectedDevice = device;
        return _currentStream->selectDevice(device);
    }

    void RtAudioRenderer::selectDefaultDevice()
    {
        std::shared_lock<std::shared_mutex> lk(_streamMutex);
        _selectedDevice = -1;
        return _currentStream->selectDefaultDevice();
    }

    RtAudioRenderer::~RtAudioRenderer() = default;
}


