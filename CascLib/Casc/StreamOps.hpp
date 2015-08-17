#pragma once

#include <fstream>
#include "Shared/Functions.hpp"

namespace Casc
{
    static int const endian_index = std::ios_base::xalloc();

    inline std::ifstream &le(std::ifstream &stream)
    {
        stream.iword(endian_index) = 0;

        return stream;
    }

    inline std::ifstream &be(std::ifstream &stream)
    {
        stream.iword(endian_index) = 1;

        return stream;
    }

    template <typename T>
    inline std::ifstream &operator>>(std::ifstream  &input, T &value)
    {
        using namespace Shared::Functions::Endian;
        char b[sizeof(T)];
        input.read(b, sizeof(T));

        value = *reinterpret_cast<T*>(b);

        if (input.iword(endian_index) == 0)
        {
            value = readLE(value);
        }

        if (input.iword(endian_index) == 1)
        {
            value = readBE(value);
        }

        return input;
    }

    inline std::ifstream &operator>>(std::ifstream  &input, std::ifstream &func(std::ifstream &stream))
    {
        return func(input);
    }
}