#pragma once

#include <interfaces/IAudioSink.h>
#include <interfaces/IAudioSource.h>

extern "C" {
    #include <libswresample/swresample.h>
    #include <libavutil/opt.h>
}

namespace CasperTech
{
    class SampleRateConverter: public IAudioSink, public IAudioSource
    {
        public:
            SampleRateConverter();

            ~SampleRateConverter() override;

            /* <IAudioNode> */
            SampleFormatFlags getSupportedSampleFormats() override;

            std::vector<uint32_t> getSupportedSampleRates() override;

            uint8_t getSupportedChannels() override;
            /* </IAudioNode> */

            /* <IAudioSink> */
            void audio(uint8_t* buffer, uint64_t sampleCount) override;
            void onSourceConfigured() override;
            /* </IAudioSink> */

            /* <IAudioSource> */
            void onSinkConfigured() override;
            void onEos() override;
            /* </IAudioSource> */

            void init();

            static AVSampleFormat getSwrSampleFormat(SampleFormatFlags sampleFormat);
            static int checkError(int errnum);

        private:
            SwrContext* _swrCtx;
            bool _sinkConfigured = false;
            bool _sourceConfigured = false;
            bool _configured = false;
            int64_t _maxDstSamples = 0;
            int _dstLineSize = 0;
            uint8_t** _dstData = nullptr;
            AVSampleFormat _srcFormat = AV_SAMPLE_FMT_FLT;
            AVSampleFormat _destFormat = AV_SAMPLE_FMT_FLT;
    };
}
