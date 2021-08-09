#pragma once

#include <RtAudio.h>

#include "RingBuffer.h"

#include <map>
#include <memory>
#include <enums/SampleFormatFlags.h>

namespace CasperTech
{
    struct AudioCallbackContainer;
    class RtAudioStream
    {
        public:
            RtAudioStream();
            ~RtAudioStream();

            std::map<uint32_t, std::string> getDevices();
            void selectDevice(uint32_t device);
            void selectDefaultDevice();
            void onEos();
            void audio(const uint8_t* buffer, const uint8_t* planarChannel, uint64_t sampleCount);
            void configure(RtAudioFormat fmt, uint8_t channels, uint32_t sampleRate, uint8_t sampleSize, uint32_t bufFrames, uint32_t bufSize);
            [[nodiscard]] SampleFormatFlags getSupportedSampleFormats() const;
            [[nodiscard]] std::vector<uint32_t> getSupportedSampleRates() const;
            [[nodiscard]] uint8_t getSupportedChannels() const;

        private:
            static int fillBufferStatic(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                                        double streamTime, RtAudioStreamStatus status, void *data);
            int fillBuffer(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                           double streamTime, RtAudioStreamStatus status);

            int _selectedDeviceId = 0;
            std::unique_ptr<RtAudio> _rtAudio;
            RtAudio::DeviceInfo _selectedDevice;
            std::unique_ptr<RingBuffer> _ringBuffer;

            uint8_t _sampleSize = 0;
            uint8_t _sourceChannels = 0;
            uint32_t _bufMs = 100;
            AudioCallbackContainer* _container = nullptr;
    };
}