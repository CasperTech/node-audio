#include "RtAudioStream.h"

namespace CasperTech
{

    RtAudioStream::RtAudioStream()
        : _ringBuffer(std::make_unique<RingBuffer>(16384))
        , _rtAudio(std::make_unique<RtAudio>())
    {
        selectDefaultDevice();
    }

    RtAudioStream::~RtAudioStream()
    {
        _ringBuffer->shutdown();
        if(_rtAudio->isStreamRunning())
        {
            _rtAudio->abortStream();
        }
        if (_rtAudio->isStreamOpen())
        {
            _rtAudio->closeStream();
        }
    }

    std::map<uint32_t, std::string> RtAudioStream::getDevices()
    {
        std::map<uint32_t, std::string> devices;
        uint32_t deviceCount = _rtAudio->getDeviceCount();
        RtAudio::DeviceInfo info;
        for(uint32_t i = 0; i < deviceCount; i++)
        {
            try
            {
                info = _rtAudio->getDeviceInfo(i);
                devices[i] = info.name;
            }
            catch(RtAudioError&)
            {

            }
        }
        return devices;
    }

    void RtAudioStream::selectDevice(uint32_t device)
    {
        try
        {
            _selectedDevice = _rtAudio->getDeviceInfo(device);
            _selectedDeviceId = device;
        }
        catch(RtAudioError&)
        {

        }
    }

    void RtAudioStream::selectDefaultDevice()
    {
        uint32_t deviceCount = _rtAudio->getDeviceCount();
        RtAudio::DeviceInfo info;
        for(uint32_t i = 0; i < deviceCount; i++)
        {
            try
            {
                info = _rtAudio->getDeviceInfo(i);
                // std::cout << "Checking " << info.name << ", " << info.outputChannels << " output channels" << std::endl;
                if (info.isDefaultOutput && info.outputChannels > 0)
                {
                    _selectedDevice = info;
                    _selectedDeviceId = i;
                    return;
                }
            }
            catch(RtAudioError&)
            {

            }
        }
        std::cout << "No suitable default device found!" << std::endl;
    }

    int RtAudioStream::fillBufferStatic(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                                        double streamTime, RtAudioStreamStatus status, void* data)
    {
        auto renderer = static_cast<RtAudioStream*>(data);
        return renderer->fillBuffer(outputBuffer, inputBuffer, nBufferFrames, streamTime, status);
    }

    int RtAudioStream::fillBuffer(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime,
                                  RtAudioStreamStatus status)
    {
        uint64_t bytesToCopy = nBufferFrames * _sampleSize *  _sourceChannels;
        if (_ringBuffer->get(reinterpret_cast<uint8_t*>(outputBuffer), bytesToCopy))
        {
            return 1;
        }
        return 0;
    }

    void RtAudioStream::onEos()
    {
        _ringBuffer->eos();
    }

    SampleFormatFlags RtAudioStream::getSupportedSampleFormats() const
    {
        auto flags = SampleFormatFlags::None;
        RtAudioFormat fmt = _selectedDevice.nativeFormats;
        if (fmt & RTAUDIO_SINT8)
        {
            flags = flags | SampleFormatFlags::S8;
        }
        else if (fmt & RTAUDIO_SINT16)
        {
            flags = flags | SampleFormatFlags::S16;
        }
        else if (fmt & RTAUDIO_SINT24)
        {
            flags = flags | SampleFormatFlags::S24;
        }
        else if (fmt & RTAUDIO_SINT32)
        {
            flags = flags | SampleFormatFlags::S32;
        }
        else if (fmt & RTAUDIO_FLOAT32)
        {
            flags = flags | SampleFormatFlags::FLT;
        }
        else if (fmt & RTAUDIO_FLOAT64)
        {
            flags = flags | SampleFormatFlags::DBL;
        }
        return flags;
    }

    std::vector<uint32_t> RtAudioStream::getSupportedSampleRates() const
    {
        return _selectedDevice.sampleRates;
    }

    uint8_t RtAudioStream::getSupportedChannels() const
    {
        return static_cast<uint8_t>(_selectedDevice.outputChannels);
    }

    void RtAudioStream::audio(uint8_t* buffer, uint64_t sampleCount)
    {
        uint64_t byteSize = sampleCount * _sampleSize * _sourceChannels;
        _ringBuffer->put(buffer, byteSize);

    }

    void RtAudioStream::configure(RtAudioFormat fmt, uint8_t channels, uint32_t sampleRate, uint8_t sampleSize,
                                  uint32_t bufFrames, uint32_t bufSize)
    {
        _sampleSize = sampleSize;
        _sourceChannels = channels;
        RtAudio::StreamParameters params;
        params.deviceId = _selectedDeviceId;
        params.firstChannel = 0;
        params.nChannels = channels;
        _ringBuffer = std::make_unique<RingBuffer>(bufSize);
        RtAudio::StreamOptions options;
        _rtAudio->openStream(&params, nullptr, fmt, sampleRate, &bufFrames, &RtAudioStream::fillBufferStatic, this, &options, nullptr);
        _rtAudio->startStream();
    }
}