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

#include <vector>
#include "Common.hpp"

namespace Casc
{
    using namespace Casc::Shared;

    class CascChunkDescriptor
    {
    private:
        CompressionMode mode_;
        size_t begin_;
        size_t count_;

    public:
        CascChunkDescriptor(CompressionMode mode, size_t begin, size_t count)
            : mode_(mode), begin_(begin), count_(count)
        {

        }

        decltype(auto) mode() const
        {
            return mode_;
        }

        decltype(auto) begin() const
        {
            return begin_;
        }

        decltype(auto) count() const
        {
            return count_;
        }
    };

    class CascLayoutDescriptor
    {
    private:
        std::vector<CascChunkDescriptor> chunks_;

    public:
        CascLayoutDescriptor(std::vector<CascChunkDescriptor> descriptors = {})
            : chunks_(descriptors)
        {

        }

        decltype(auto) chunks() const
        {
            return (chunks_);
        }
    };
}
