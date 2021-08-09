#pragma once

#include <interfaces/IAudioSink.h>
#include <interfaces/IAudioSource.h>

extern "C" {
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
}

#include <atomic>

namespace CasperTech
{
    class VolumeFilter: public IAudioSink, public IAudioSource
    {
        public:
            VolumeFilter();

            ~VolumeFilter() override;

            /* <IAudioNode> */
            SampleFormatFlags getSupportedSampleFormats() override;

            std::vector<uint32_t> getSupportedSampleRates() override;

            uint8_t getSupportedChannels() override;
            std::string getName() const override;
            /* </IAudioNode> */

            /* <IAudioSink> */
            void audio(const uint8_t* buffer, const uint8_t* planarChannel, uint64_t sampleCount) override;
            void onSourceConfigured() override;
            /* </IAudioSink> */

            /* <IAudioSource> */
            void onSinkConfigured() override;
            void onEos() override;
            /* </IAudioSource> */

            void setVolume(float volume);

            void init();

            static AVSampleFormat getAvSampleFormat(SampleFormatFlags sampleFormat);
            static int checkError(int errnum);

        private:
            bool isPlanar(int fmt);

            std::mutex _pipelineMutex;

            AVFrame* _frame;
            AVFilterGraph* _filterGraph = nullptr;
            const AVFilter* _aBufferFilter = nullptr;
            AVFilterContext* _aBufferCtx = nullptr;
            AVFilterContext* _aBufferSinkCtx = nullptr;
            const AVFilter* _aBufferSink = nullptr;
            AVFilterContext* _volumeCtx = nullptr;
            const AVFilter*  _volumeFilter;
            AVSampleFormat _avSampleFormat;
            uint64_t _avChannelLayout;
            uint64_t _pts = 0;
            uint8_t _sampleSize = 0;
            float _volume = 1.0;


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
