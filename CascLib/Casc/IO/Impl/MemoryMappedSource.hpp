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

#include "../DataSource.hpp"

namespace Casc
{
    namespace IO
    {
        namespace Impl
        {
            /**
             * A source for data.
             */
            class MemoryMappedSource : public DataSource
            {
                std::vector<char> buf;

            public:
                /**
                 * Constructor.
                 */
                MemoryMappedSource(std::vector<char> bytes)
                    : DataSource(DataSourceType::MemoryMapped), buf(bytes) { }

                /**
                 * Gets a chunk of data.
                 */
                std::vector<char> get(size_t offset, size_t count) override
                {
                    auto begin = buf.begin() + offset;
                    auto end = size_t(buf.end() - begin) > count ? begin + count : buf.end();

                    return std::vector<char>(begin, end);
                };

                using DataSource::DataSource;
            };
        }
    }
}