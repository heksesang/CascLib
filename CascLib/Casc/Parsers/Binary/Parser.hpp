/*
* Copyright 2016 Gunnar Lilleaasen
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

#include <memory>
#include <iostream>
#include <tuple>
#include <vector>

#include "../../Common.hpp"
#include "../../Exceptions.hpp"

namespace Casc::Parsers::Binary
{
    template <typename Ty> class Parser;

    template <>
    class Parser<std::string>
    {
        size_t readBlockSize(std::shared_ptr<std::istream> stream) const
        {
            char b[sizeof(uint32_t)];
            stream->read(b, sizeof(uint32_t));

            return IO::Endian::read<IO::EndianType::Big, uint32_t>(b);
        }

    public:
        std::string readOne(std::shared_ptr<std::istream> stream) const
        {
            std::string profile;
            std::getline(*stream, profile, '\0');

            if (stream->fail())
            {
                throw Exceptions::IOException("Stream faulted after getline()");
            }

            return profile;
        }

        std::vector<std::string> readMany(std::shared_ptr<std::istream> stream) const
        {
            std::vector<std::string> strings;

            size_t stringBlockSize = readBlockSize(stream);

            std::streamoff profileTableBegin = stream->tellg();

            while (stream->tellg() <= std::streampos(profileTableBegin + readBlockSize(stream)))
            {
                strings.emplace_back(readOne(stream));
            }

            return strings;
        }
    };
}