#include "RtAudioStream.h"
#include "AudioCallbackContainer.h"

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
        if (_container != nullptr)
        {
            std::unique_lock<std::mutex> lk(_container->containerMutex);
            _container->rtAudioStream = nullptr;
        }
        std::unique_lock<std::mutex> _fillBufferMutex;
        _ringBuffer->shutdown();
        if(_rtAudio->isStreamRunning())
        {
#ifdef _DEBUG
            std::cout << "Aborting Stream " << std::endl;
#endif
            _rtAudio->abortStream();
        }
        if (_rtAudio->isStreamOpen())
        {
#ifdef _DEBUG
            std::cout << "Closing Stream " << std::endl;
#endif
            _rtAudio->closeStream();
        }
#ifdef _DEBUG
        std::cout << "RtAudioStreamDestroyed " << std::endl;
#endif
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
#ifdef _DEBUG
                std::cout << "Searching for default devie: Checking " << info.name << ", " << info.outputChannels << " output channels" << std::endl;
#endif
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
#ifdef _DEBUG
        std::cout << "No suitable default device found!" << std::endl;
#endif
    }

    int RtAudioStream::fillBufferStatic(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                                        double streamTime, RtAudioStreamStatus status, void* data)
    {
        auto container = static_cast<AudioCallbackContainer*>(data);
        RtAudioStream* stream = nullptr;
        {
            std::unique_lock<std::mutex> lk(container->containerMutex);
            stream = container->rtAudioStream;
            if (!stream)
            {
                delete container;
                return 2;
            }
        }
        return stream->fillBuffer(outputBuffer, inputBuffer, nBufferFrames, streamTime, status);
    }

    int RtAudioStream::fillBuffer(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime,
                                  RtAudioStreamStatus status)
    {
        std::unique_lock<std::mutex> _fillBufferMutex;
        uint64_t bytesToCopy = nBufferFrames * _sampleSize *  _sourceChannels;
        if (_ringBuffer->get(reinterpret_cast<uint8_t*>(outputBuffer), bytesToCopy))
        {
#ifdef _DEBUG
            std::cout << "RingBuffer shutdown" << std::endl;
#endif
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
        if (fmt & RTAUDIO_SINT16)
        {
            flags = flags | SampleFormatFlags::S16;
        }
        if (fmt & RTAUDIO_SINT24)
        {
            flags = flags | SampleFormatFlags::S24;
        }
        if (fmt & RTAUDIO_SINT32)
        {
            flags = flags | SampleFormatFlags::S32;
        }
        if (fmt & RTAUDIO_FLOAT32)
        {
            flags = flags | SampleFormatFlags::FLT;
        }
        if (fmt & RTAUDIO_FLOAT64)
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

    void RtAudioStream::audio(const uint8_t* buffer, const uint8_t* planarChnanel, uint64_t sampleCount)
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
#ifdef _DEBUG
        std::cout << "Starting RtAudioStream with " << unsigned(channels) << " channels, sample rate " << sampleRate << std::endl;
#endif

        _container = new AudioCallbackContainer();
        _container->rtAudioStream = this;
        _rtAudio->openStream(&params, nullptr, fmt, sampleRate, &bufFrames, &RtAudioStream::fillBufferStatic, _container, &options, nullptr);
        _rtAudio->startStream();
    }
}