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
#include "../../Exceptions.hpp"

namespace Casc
{
    namespace IO
    {
        namespace Impl
        {
            /**
             * A source for data.
             */
            class StreamSource : public DataSource
            {
                std::shared_ptr<std::istream> stream;
                size_t begin;
                size_t end;

            public:
                /**
                 * Constructor.
                 */
                StreamSource(std::shared_ptr<std::istream> stream, std::pair<size_t, size_t> bounds) :
                    DataSource(DataSourceType::Stream), stream(stream),
                    begin(bounds.first), end(bounds.second) { }

                /**
                 * Gets a chunk of data.
                 */
                std::vector<char> get(size_t offset, size_t count) override
                {
                    if (offset >= (end - begin))
                    {
                        throw Exceptions::IOException("Invalid offset");
                    }

                    auto available = end - begin - offset;

                    if (count > available)
                    {
                        count = available;
                    }

                    std::vector<char> v(count, '\0');
                    stream->seekg(begin + offset, std::ios_base::beg);
                    stream->read(v.data(), count);

                    return v;
                }

                using DataSource::DataSource;
            };
        }
    }
}