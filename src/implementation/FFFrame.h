#pragma once

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

namespace CasperTech
{
    struct FFFrame
    {
        AVFrame* frame = nullptr;

        FFFrame()
        {
            frame = av_frame_alloc();
        }
        ~FFFrame()
        {
            if (frame != nullptr)
            {
                av_frame_free(&frame);
                frame = nullptr;
            }
        }
    };
}
