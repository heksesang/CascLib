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
             * Zlib handler. This decompresses a zlib compressed chunk and extracts the data.
             */
            class ZlibHandler : public Handler
            {
                const int CompressionLevel = 9;
                const int WindowBits = 15;

                ZStreamBase::char_t* out = nullptr;
                size_t avail_out = 0;

                EncodingMode mode() const override
                {
                    return EncodingMode::Zlib;
                }

                std::unique_ptr<char[]> read(std::filebuf &buf, std::filebuf::off_type offset, size_t inSize, size_t outSize, std::filebuf::off_type &chunkSize) override
                {
                    if (offset == 0 || avail_out - offset < outSize)
                    {
                        auto in = std::make_unique<ZStreamBase::char_t[]>(inSize);
                        if (buf.sgetn(reinterpret_cast<char*>(in.get()), inSize) == inSize)
                        {
                            ZInflateStream(in.get(), inSize).readAll(&out, avail_out);
                            chunkSize = avail_out;
                        }
                        else
                        {
                            return nullptr;
                        }
                    }

                    ZStreamBase::char_t *resizedOut = new ZStreamBase::char_t[outSize];
                    std::memcpy(resizedOut, out + offset, outSize);

                    return std::unique_ptr<char[]>(reinterpret_cast<char*>(resizedOut));
                }

                std::vector<char> write(std::istream &stream, size_t inSize) const override
                {
                    std::vector<char> v(inSize + 1, '\0');
                    v[0] = mode();
                    stream.read(&v[1], inSize);

                    ZDeflateStream zstream(this->CompressionLevel);
                    zstream.write(reinterpret_cast<ZStreamBase::char_t*>(&v[0]), inSize);

                    char* buf;
                    size_t bufSize;

                    zstream.readAll(reinterpret_cast<ZStreamBase::char_t**>(&buf), bufSize);

                    return std::vector<char>(buf, buf + bufSize);
                }

            public:
                virtual ~ZlibHandler() override
                {
                    delete[] out;
                }
            };
        }
    }
}