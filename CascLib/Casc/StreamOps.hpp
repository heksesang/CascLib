/*
* Copyright 2014 Gunnar Lilleaasen
*
* This file is part of CascLib.
*
* CascLib is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* CascLib is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CascLib.  If not, see <http://www.gnu.org/licenses/>.
*/

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

    inline std::ofstream &le(std::ofstream &stream)
    {
        stream.iword(endian_index) = 0;

        return stream;
    }

    inline std::ifstream &be(std::ifstream &stream)
    {
        stream.iword(endian_index) = 1;

        return stream;
    }

    inline std::ofstream &be(std::ofstream &stream)
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

    template <typename T>
    inline std::ofstream &operator<<(std::ofstream  &input, T &value)
    {
        using namespace Shared::Functions::Endian;

        if (input.iword(endian_index) == 0)
        {
            input.write(writeLE<T>(value).data(), sizeof(T));
        }

        if (input.iword(endian_index) == 1)
        {
            input.write(writeBE<T>(value).data(), sizeof(T));
        }

        return input;
    }

    inline std::ifstream &operator>>(std::ifstream  &input, std::ifstream &func(std::ifstream &stream))
    {
        return func(input);
    }

    inline std::ofstream &operator>>(std::ofstream  &input, std::ofstream &func(std::ofstream &stream))
    {
        return func(input);
    }

    inline std::ifstream &operator<<(std::ifstream  &input, std::ifstream &func(std::ifstream &stream))
    {
        return func(input);
    }

    inline std::ofstream &operator<<(std::ofstream  &input, std::ofstream &func(std::ofstream &stream))
    {
        return func(input);
    }
}