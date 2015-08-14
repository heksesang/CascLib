#pragma once

#include <fstream>
#include "Shared/Functions.hpp"

namespace Casc
{
    extern int const endian_index;

    std::ifstream &le(std::ifstream &stream)
    {
        stream.iword(endian_index) = 0;

        return stream;
    }

    std::ifstream &be(std::ifstream &stream)
    {
        stream.iword(endian_index) = 1;

        return stream;
    }

    template <typename T>
    std::ifstream &operator>>(std::ifstream  &input, T &value)
    {
        using namespace Functions::Endian;
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

    std::ifstream &operator>>(std::ifstream  &input, std::ifstream &func(std::ifstream &stream))
    {
        return func(input);
    }
}