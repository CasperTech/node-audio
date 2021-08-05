#pragma once

#include <type_traits>
#include <iostream>

enum class SampleFormatFlags
{
        None = 0,
        U8 = 1 << 0,
        S8 = 1 << 1,
        S16 = 1 << 2,
        S24 = 1 << 3,
        S32 = 1 << 4,
        FLT = 1 << 5,
        DBL = 1 << 6,

        U8_Planar = 1 << 7,
        S8_Planar = 1 << 8,
        S16_Planar = 1 << 9,
        S24_Planar = 1 << 10,
        S32_Planar = 1 << 11,
        FLT_Planar = 1 << 12,
        DBL_Planar = 1 << 13,

        LAST_FLAG = 1 << 14
};

inline SampleFormatFlags operator | (SampleFormatFlags lhs, SampleFormatFlags rhs)
{
    using T = std::underlying_type<SampleFormatFlags>::type;
    return static_cast<SampleFormatFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline SampleFormatFlags& operator |= (SampleFormatFlags& lhs, SampleFormatFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline bool operator != (const SampleFormatFlags& lhs, const int rhs)
{
    return static_cast<int>(lhs) != rhs;
}

inline bool operator != (const SampleFormatFlags& lhs, const unsigned int rhs)
{
    return static_cast<int>(lhs) != rhs;
}

inline bool operator < (const SampleFormatFlags& lhs, const unsigned int rhs)
{
    return static_cast<unsigned int>(lhs) < rhs;
}

inline bool operator > (const SampleFormatFlags& lhs, const unsigned int rhs)
{
    return static_cast<unsigned int>(lhs) > rhs;
}

inline bool operator < (const unsigned int lhs, const SampleFormatFlags& rhs)
{
    return lhs < static_cast<unsigned int>(rhs);
}

inline bool operator > (const unsigned int lhs, const SampleFormatFlags& rhs)
{
    return lhs > static_cast<unsigned int>(rhs);
}

inline bool operator < (const SampleFormatFlags& lhs, const int rhs)
{
    return static_cast<int>(lhs) < rhs;
}

inline bool operator > (const SampleFormatFlags& lhs, const int rhs)
{
    return static_cast<int>(lhs) > rhs;
}

inline bool operator < (const int lhs, const SampleFormatFlags& rhs)
{
    return lhs < static_cast<int>(rhs);
}

inline bool operator > (const int lhs, const SampleFormatFlags& rhs)
{
    return lhs > static_cast<int>(rhs);
}

inline SampleFormatFlags operator & (SampleFormatFlags lhs, SampleFormatFlags rhs)
{
    using T = std::underlying_type<SampleFormatFlags>::type;
    return static_cast<SampleFormatFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline SampleFormatFlags& operator &= (SampleFormatFlags& lhs, SampleFormatFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline std::ostream& operator<<(std::ostream& str, SampleFormatFlags type)
{
    int x = 0;
    if (type == SampleFormatFlags::None)
    {
        str << "None";
        return str;
    }
    bool first = true;
    while(x < SampleFormatFlags::LAST_FLAG)
    {
        if ((type & static_cast<SampleFormatFlags>(x)) != 0)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                str << ", ";
            }
            switch(static_cast<SampleFormatFlags>(x))
            {
                case SampleFormatFlags::U8:
                    str << "U8";
                    break;
                case SampleFormatFlags::S8:
                    str << "S8";
                    break;
                case SampleFormatFlags::S16:
                    str << "S16";
                    break;
                case SampleFormatFlags::S24:
                    str << "S24";
                    break;
                case SampleFormatFlags::S32:
                    str << "S32";
                    break;
                case SampleFormatFlags::FLT:
                    str << "FLT";
                    break;
                case SampleFormatFlags::DBL:
                    str << "DBL";
                    break;
                case SampleFormatFlags::U8_Planar:
                    str << "U8P";
                    break;
                case SampleFormatFlags::S8_Planar:
                    str << "S8P";
                    break;
                case SampleFormatFlags::S16_Planar:
                    str << "S16P";
                    break;
                case SampleFormatFlags::S24_Planar:
                    str << "S24P";
                    break;
                case SampleFormatFlags::S32_Planar:
                    str << "S32P";
                    break;
                case SampleFormatFlags::FLT_Planar:
                    str << "FLTP";
                    break;
                case SampleFormatFlags::DBL_Planar:
                    str << "DBLP";
                    break;
                case SampleFormatFlags::LAST_FLAG:
                    [[fallthrough]];
                case SampleFormatFlags::None:
                    break;
            }
        }
        if (x == 0)
        {
            x = 1;
        }
        else
        {
            x = x * 2;
        }
    }
    return str;
}

