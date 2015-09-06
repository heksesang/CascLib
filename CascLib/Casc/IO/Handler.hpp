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

#include "../zlib.hpp"
#include "../Common.hpp"

#include "EncodingMode.hpp"

namespace Casc
{
    namespace IO
    {
        /**
         * Base class for block handlers.
         *
         * Classes derived from this can be used to implement
         * different compression algorithms or encryptions.
         */
        class Handler
        {
        public:
            virtual ~Handler() { }

            /**
             * The compression mode this handler should be registered for.
             */
            virtual EncodingMode mode() const = 0;

            /**
             * Reads and processes data from the file buffer and returns the result.
             *
             * @param buf		The file buffer to read data from.
             * @param offset	The offset to read at.
             * @param inSize	The number of bytes to read from the stream.
             * @param outSize	The number of bytes to return.
             *
             * @return A pointer to the byte array.
             */
            virtual std::unique_ptr<char[]> read(std::filebuf &buf, std::filebuf::off_type offset, size_t inSize, size_t outSize, std::filebuf::off_type &chunkSize) = 0;

            virtual std::vector<char> write(std::istream &stream, size_t inSize) const = 0;
        };

        /**
        * Default handler. This reads data directly from the stream.
        */
        class DefaultHandler : public Handler
        {
            EncodingMode mode() const override
            {
                return EncodingMode::None;
            }

            std::unique_ptr<char[]> read(std::filebuf &buf, std::filebuf::off_type offset, size_t inSize, size_t outSize, std::filebuf::off_type &chunkSize) override
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

            std::vector<char> write(std::istream &stream, size_t inSize) const override
            {
                std::vector<char> v(inSize + 1, '\0');
                v[0] = mode();
                stream.read(&v[1], inSize);

                return std::move(v);
            }
        };

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