#pragma once

#include <enums/SampleFormatFlags.h>

#include <map>
#include <vector>

namespace CasperTech
{
    class IAudioRenderer
    {
        public:
            virtual std::map<uint32_t, std::string> getDevices() = 0;
            virtual void selectDevice(uint32_t device) = 0;
            virtual void selectDefaultDevice() = 0;
            virtual ~IAudioRenderer() = default;
    };
}