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

#include "../Common.hpp"

namespace Casc
{
    namespace Filesystem
    {
        class Root
        {
            std::shared_ptr<Parsers::Binary::Encoding> encoding;
            std::shared_ptr<Parsers::Binary::Index> index;
            std::shared_ptr<IO::StreamAllocator> allocator;

        public:
            Root(Hex hash, std::shared_ptr<Parsers::Binary::Encoding> encoding = nullptr,
                 std::shared_ptr<Parsers::Binary::Index> index = nullptr,
                 std::shared_ptr<IO::StreamAllocator> allocator = nullptr)
                : encoding(encoding), index(index), allocator(allocator)
            {
            }

            std::string find(std::string path)
            {
                return "";
            }
        };
    }
}