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

#ifdef _MSC_VER
#include <experimental/filesystem>
#else
#include <boost/filesystem.hpp>
#endif

template<class Facet>
struct deletable_facet : Facet
{
    template<class ...Args>
    deletable_facet(Args&& ...args) : Facet(std::forward<Args>(args)...) {}
    ~deletable_facet() {}
};

namespace Casc
{
    class CascBuildInfo;
    class CascConfiguration;
    class CascEncoding;
    class CascEncodingBlock;
    class CascIndex;
    class CascLayoutDescriptor;
    class CascRootHandler;
    class CascShmem;
    class CascReference;

    /* BLTE handler */
    template <typename Traits>
    class BaseCascBlteHandler;
    typedef BaseCascBlteHandler<std::filebuf::traits_type> CascBlteHandler;

    /* Buffer */
    template <size_t BufferSize>
    class BaseCascBuffer;
    typedef BaseCascBuffer<4096U> CascBuffer;

    /* Stream */
    template <bool Writeable>
    class CascStream;


    /* Container */
    class CascContainer;

#ifdef _MSC_VER
    namespace fs = std::experimental::filesystem::v1;
#else
    namespace fs = boost::filesystem;
#endif
}

#include "StreamOps.hpp"

#include "Exceptions/CascException.hpp"
#include "Exceptions/EncodingProfileParserException.hpp"
#include "Exceptions/FileDoesNotExistException.hpp"
#include "Exceptions/FileNotFoundException.hpp"
#include "Exceptions/GenericException.hpp"
#include "Exceptions/InvalidHashException.hpp"
#include "Exceptions/InvalidSignatureException.hpp"
#include "Exceptions/NoFreeSpaceException.hpp"

using namespace Casc::Exceptions;

#include "Shared/CompressionMode.hpp"
#include "Shared/FileSearch.hpp"
#include "Shared/Functions.hpp"
#include "Shared/Hex.hpp"

namespace std
{
    template <>
    class std::hash<std::array<uint8_t, 9>>
    {
    public:
        size_t operator()(const std::array<uint8_t, 9> &key) const
        {
            return Casc::Shared::Functions::Hash::lookup3(key, 0);
        }
    };

    template <>
    class std::hash<std::vector<char>>
    {
    public:
        size_t operator()(const std::vector<char> &key) const
        {
            return Casc::Shared::Functions::Hash::lookup3(key, 0);
        }
    };
}

#include "CascEncodingBlock.hpp"
#include "CascReference.hpp"
#include "CascBlteHandler.hpp"
#include "CascBuffer.hpp"
#include "CascBuildInfo.hpp"
#include "CascConfiguration.hpp"
#include "CascContainer.hpp"
#include "CascEncoding.hpp"
#include "CascIndex.hpp"
#include "CascRootHandler.hpp"
#include "CascShmem.hpp"
#include "CascStream.hpp"