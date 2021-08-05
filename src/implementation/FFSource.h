#pragma once

#include <string>
#include <interfaces/IAudioSource.h>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}

#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)

namespace CasperTech
{
    struct FFFrame;
    class FFSource: public IAudioSource
    {
        public:
            static std::string getError(int errnum);
            FFSource();
            ~FFSource() noexcept override;
            void load(const std::string& fileName);
            void seek(uint64_t timeMs);
            int getPacket(FFFrame * frame);

            /* <IAudioNode> */
            SampleFormatFlags getSupportedSampleFormats() override;
            std::vector<uint32_t> getSupportedSampleRates() override;
            uint8_t getSupportedChannels() override;
            /* </IAudioNode> */

        private:
            static void checkError(int errnum);

            AVFormatContext* _fmtCtx = nullptr;
            AVCodecContext* _audioCtx = nullptr;
            AVPacket _pkt = {};


            AVCodecParameters* _audioCodec = nullptr;
            AVStream* _stream = nullptr;
            bool _packetInitialised = false;

            uint32_t _streamIndex = 0;
            uint32_t _channels = 0;
            uint32_t _sampleRate = 0;
    };
}

