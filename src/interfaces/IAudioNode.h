#pragma once

#include <enums/SampleFormatFlags.h>

#include <map>
#include <vector>
#include <memory>

class IAudioNode
{
    public:
        virtual std::string getName() const = 0;
        virtual SampleFormatFlags getSupportedSampleFormats() = 0;
        virtual std::vector<uint32_t> getSupportedSampleRates() = 0;
        virtual uint8_t getSupportedChannels() = 0;
};

