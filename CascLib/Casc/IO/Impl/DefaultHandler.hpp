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

#include "../../zlib.hpp"
#include "../../Common.hpp"

#include "../EncodingMode.hpp"

namespace Casc
{
    namespace IO
    {
        namespace Impl
        {
            /**
             * Default handler. This reads data directly from the stream.
             */
            class DefaultHandler : public Handler
            {
                EncodingMode mode() const override
                {
                    return EncodingMode::None;
                }

                std::unique_ptr<char[]> decode(std::filebuf &buf, std::filebuf::off_type offset, size_t inSize, size_t outSize, std::filebuf::off_type &chunkSize) override
                {
                    auto out = std::make_unique<char[]>(outSize);

                    if (offset > 0)
                        buf.pubseekoff(offset, std::ios_base::cur);

                    if (buf.sgetn(out.get(), outSize) == outSize)
                    {
                        return out;
                    }

                    return nullptr;
                }

                std::vector<char> encode(std::istream &stream, size_t inSize) const override
                {
                    std::vector<char> v(inSize + 1, '\0');
                    v[0] = mode();
                    stream.read(&v[1], inSize);

                    return std::move(v);
                }
            };
        }
    }
}