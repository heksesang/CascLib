/*
* Copyright 2015 Gunnar Lilleaasen
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

#include <locale>
#include <utility>
#include <iostream>
#include <codecvt>

// Needed for wchar_t > char_t conversion
template<class Facet>
struct deletable_facet : Facet
{
    template<class ...Args>
    deletable_facet(Args&& ...args) : Facet(std::forward<Args>(args)...) {}
    ~deletable_facet() {}
};

// Define boost or std <filesystem> header
#ifdef _MSC_VER
#include <experimental/filesystem>
#else
#include <boost/filesystem.hpp>
#endif

namespace Casc
{
#ifdef _MSC_VER
    namespace fs = std::experimental::filesystem::v1;
#else
    namespace fs = boost::filesystem;
#endif
}

// Forward declarations
namespace Casc
{
    namespace Exceptions
    {
        class CascException;
        
        class ParserException;
        class FilesystemException;
        class IOException;

        class FileNotFoundException;
        class FilenameDoesNotExistException;
        class HashDoesNotExistException;
        class InvalidHashException;
        class InvalidSignatureException;
        class KeyDoesNotExistException;
        class ReserveSpaceException;
    }

    namespace Filesystem
    {
        class RootHandler;
    }

    namespace IO
    {
        namespace Impl
        {
            class DefaultHandler;
            class ZlibHandler;
        }

        class Handler;
        class Buffer;
        template <bool Writeable>
        class Stream;
    }

    namespace Parsers
    {
        namespace Binary
        {
            class Shmem;
            class Encoding;
            class Index;
            class Reference;
        }

        namespace Text
        {
            class BuildInfo;
            class Configuration;
            class EncodingBlock;
        }
    }

    class Container;
}

// Enums
#include "IO/EncodingMode.hpp"
#include "IO/EndianType.hpp"

// Helpers
#include "Shared/Functions.hpp"
#include "Shared/Hex.hpp"

/**
* Stream operators
*/
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
        using namespace Functions::Endian;
        char b[sizeof(T)];
        input.read(b, sizeof(T));

        value = *reinterpret_cast<T*>(b);

        if (input.iword(endian_index) == 0)
        {
            value = read<IO::EndianType::Little, uint32_t>(b, b + sizeof(T));
        }

        if (input.iword(endian_index) == 1)
        {
            value = read<IO::EndianType::Big, uint32_t>(b, b + sizeof(T));
        }

        return input;
    }

    template <typename T>
    inline std::ofstream &operator<<(std::ofstream  &input, const T &value)
    {
        using namespace Functions::Endian;

        if (input.iword(endian_index) == 0)
        {
            input.write(write<IO::EndianType::Little>(value).data(), sizeof(T));
        }

        if (input.iword(endian_index) == 1)
        {
            input.write(write<IO::EndianType::Big>(value).data(), sizeof(T));
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

// Hash algorithm for unordered_map
namespace std
{
    template <>
    class std::hash<std::array<uint8_t, 9>>
    {
    public:
        size_t operator()(const std::array<uint8_t, 9> &key) const
        {
            return Casc::Functions::Hash::lookup3(key, 0);
        }
    };

    template <>
    class std::hash<::std::vector<char>>
    {
    public:
        size_t operator()(const std::vector<char> &key) const
        {
            return Casc::Functions::Hash::lookup3(key, 0);
        }
    };
}

#include "Container.hpp"