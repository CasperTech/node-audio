#pragma once

extern "C" {
    #include <libavcodec/avcodec.h>
}

namespace CasperTech
{
    class AVPacket;
    class ScopedPacketUnref
    {
        public:
            explicit ScopedPacketUnref(::AVPacket* pkt);
            ~ScopedPacketUnref();

        private:
            ::AVPacket* _pkt;
    };
}