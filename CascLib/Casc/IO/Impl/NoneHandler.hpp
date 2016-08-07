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
#include <memory>
#include <cstring>

#include "../../zlib.hpp"
#include "../../md5.hpp"

#include "../../Exceptions.hpp"

namespace Casc
{
    namespace IO
    {
        namespace Impl
        {
            /**
             * Default handler. This reads data directly from the stream.
             */
            class NoneHandler : public Handler
            {
            public:
                EncodingMode mode() const override
                {
                    return EncodingMode::None;
                }

                std::vector<char> decode(size_t offset, size_t count) override
                {
                    return source->get(offset + 1, count);
                }

                std::vector<char> encode(std::vector<char> input) const override
                {
                    std::vector<char> v(input.size() + 1, '\0');
                    std::memcpy(v.data() + 1, input.data(), input.size());
                    v[0] = mode();

                    return std::move(v);
                }

                size_t logicalSize() override
                {
                    return chunk.size - 1;
                }

                void reset() override
                {

                }

                NoneHandler(std::shared_ptr<DataSource> source) :
                    Handler({
                        0, source->upper_bound - source->lower_bound - 1,
                        0, source->upper_bound - source->lower_bound },
                    source)
                {

                }

                using Handler::Handler;
            };
        }
    }
}