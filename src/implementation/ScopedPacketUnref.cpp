#include "ScopedPacketUnref.h"

namespace CasperTech
{
    ScopedPacketUnref::ScopedPacketUnref(::AVPacket* pkt)
        :_pkt(pkt)
    {

    }

    ScopedPacketUnref::~ScopedPacketUnref()
    {
        av_packet_unref(_pkt);
    }
}


