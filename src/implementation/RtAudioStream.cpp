#include "RtAudioStream.h"
#include "AudioCallbackContainer.h"

namespace CasperTech
{
    std::map<uint32_t, std::string> RtAudioStream::devices;
    RtAudio::DeviceInfo RtAudioStream::defaultDevice;
    int RtAudioStream::defaultDeviceId = -1;

    RtAudioStream::RtAudioStream()
        : _ringBuffer(std::make_unique<RingBuffer>(16384))
        , _rtAudio(std::make_unique<RtAudio>())
    {
        selectDefaultDevice();
    }

    RtAudioStream::~RtAudioStream()
    {
        shutdown();
    }

    std::map<uint32_t, std::string> RtAudioStream::getDevices()
    {
        if (!devices.empty())
        {
            return devices;
        }
        devices = {};
        uint32_t deviceCount = _rtAudio->getDeviceCount();
        RtAudio::DeviceInfo info;
        for(uint32_t i = 0; i < deviceCount; i++)
        {
            try
            {
                info = _rtAudio->getDeviceInfo(i);
                if (info.isDefaultOutput && info.outputChannels > 0)
                {
                    defaultDevice = info;
                    defaultDeviceId = i;
                    _selectedDevice = info;
                    _selectedDeviceId = i;
                }
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
        if (defaultDeviceId > -1 && device == defaultDeviceId)
        {
            _selectedDevice = defaultDevice;
            _selectedDeviceId = defaultDeviceId;
            return;
        }
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
        if (defaultDeviceId > -1)
        {
            _selectedDevice = defaultDevice;
            _selectedDeviceId = defaultDeviceId;
            return;
        }
        uint32_t deviceCount = _rtAudio->getDeviceCount();
        RtAudio::DeviceInfo info;
        std::cout << "DeviceCount: " << deviceCount << std::endl;
        for(uint32_t i = 0; i < deviceCount; i++)
        {
            try
            {
                info = _rtAudio->getDeviceInfo(i);
#ifdef _DEBUG
                std::cout << "Searching for default device: Checking " << info.name << ", " << info.outputChannels << " output channels" << std::endl;
#endif
                if (info.isDefaultOutput && info.outputChannels > 0)
                {
#ifdef _DEBUG
                    std::cout << "Found default device: " << info.name << std::endl;
#endif
                    _selectedDevice = info;
                    _selectedDeviceId = i;
                    return;
                }
            }
            catch(RtAudioError& e)
            {
                std::cout << e.what() << std::endl << std::flush;
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
                return 0;
            }
        }
        int result = stream->fillBuffer(outputBuffer, inputBuffer, nBufferFrames, streamTime, status);
        if (result == 1)
        {
            {
                std::unique_lock<std::mutex> lk(container->containerMutex);
                stream = container->rtAudioStream;
                if (!stream)
                {
                    return 0;
                }
                else
                {
                    container->bufferDone = 1;
                }
            }
        }
        return 0;
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

    void RtAudioStream::shutdown()
    {
        if (_container != nullptr)
        {
            std::unique_lock<std::mutex> lk(_container->containerMutex);
            _container->rtAudioStream = nullptr;
            _ringBuffer->shutdown();
        }

        if (_rtAudio->isStreamRunning())
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
        if (_container != nullptr)
        {
            delete _container;
            _container = nullptr;
        }
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

        shutdown();
        if (_container != nullptr)
        {
            _ringBuffer->shutdown();
            delete _container;
        }

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