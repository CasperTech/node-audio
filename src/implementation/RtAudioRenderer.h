#pragma once

#include <interfaces/IAudioRenderer.h>
#include <interfaces/IAudioSink.h>

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <fstream>
#include <condition_variable>


class RtAudio;
namespace CasperTech
{
    class RtAudioStream;
    class RtAudioRenderer: public IAudioRenderer, public IAudioSink
    {
        public:
            RtAudioRenderer();
            ~RtAudioRenderer() override;

            /* <IAudioRenderer> */
            std::map<uint32_t, std::string> getDevices() override;
            void selectDevice(uint32_t device) override;
            void selectDefaultDevice() final;
            /* </IAudioRenderer> */

            /* <IAudioNode> */
            SampleFormatFlags getSupportedSampleFormats() override;
            std::vector<uint32_t> getSupportedSampleRates() override;
            uint8_t getSupportedChannels() override;
            /* </IAudioNode> */

            /* <IAudioSink> */
            void audio(uint8_t* buffer, uint64_t sampleCount) override;
            void onSourceConfigured() override;
            void onEos() override;
            /* </IAudioSink> */

        private:
            std::unique_ptr<RtAudioStream> _currentStream;
            std::shared_mutex _streamMutex;
            int32_t _selectedDevice = -1;





            bool _streamStarted = false;

            mutable std::mutex _bufMutex;
            std::condition_variable _streamWait;
            uint8_t _sampleSize = 0;
            bool _bufferFilled = false;
            uint8_t* _buffer = nullptr;
            uint64_t _bufPos = 0;
            uint64_t _bufferSize = 0;
            uint64_t _sampleCount = 0;
            uint64_t _bufferLengthMs = 50;
    };
}
