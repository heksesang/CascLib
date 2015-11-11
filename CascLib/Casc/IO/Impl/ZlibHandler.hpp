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

namespace Casc
{
    namespace IO
    {
        namespace Impl
        {
            /**
             * Zlib handler. This decompresses a zlib compressed chunk and extracts the data.
             */
            class ZlibHandler : public Handler
            {
                const int CompressionLevel = 9;
                const int WindowBits = 15;

                std::vector<char> decoded;

            public:
                EncodingMode mode() const override
                {
                    return EncodingMode::Zlib;
                }

                std::vector<char> decode(size_t offset, size_t count) override
                {
                    auto in = source->get(1, chunk.size - 1);

                    if (decoded.size() == 0)
                    {
                        ZStreamBase::char_t* out = nullptr;
                        size_t avail_out = 0;

                        ZInflateStream(reinterpret_cast<unsigned char*>(
                            in.data()), in.size()).readAll(&out, avail_out);

                        decoded.resize(avail_out);
                        std::memcpy(decoded.data(), out, avail_out);

                        delete out;
                    }

                    if (offset >= decoded.size())
                    {
                        throw Exceptions::IOException("Invalid offset.");
                    }

                    if ((decoded.size() - offset) < count)
                    {
                        //throw Exceptions::IOException("Count exceeds array bounds.");
                    }

                    auto begin = decoded.begin() + offset;
                    auto end = size_t(decoded.end() - begin) < count ? decoded.end() : begin + count;

                    return { begin, end };
                }

                std::vector<char> encode(std::vector<char> input) const override
                {
                    ZDeflateStream zstream(this->CompressionLevel);
                    zstream.write(reinterpret_cast<ZStreamBase::char_t*>(input.data()), input.size());

                    char* buf;
                    size_t bufSize;

                    zstream.readAll(reinterpret_cast<ZStreamBase::char_t**>(&buf), bufSize);

                    std::vector<char> v(bufSize + 1, '\0');
                    std::memcpy(v.data() + 1, buf, bufSize);
                    v[0] = mode();

                    return std::move(v);
                }

                size_t logicalSize() override
                {
                    return chunk.end - chunk.begin;
                }

                void reset() override
                {
                    decoded.clear();
                }

                using Handler::Handler;
            };
        }
    }
}