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

#include <array>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdint.h>
#include <vector>
#include <map>

#include "../Common.hpp"
#include "../Exceptions.hpp"

#include "../md5.hpp"
#include "../zlib.hpp"

#include "Handler.hpp"

namespace Casc
{
    namespace IO
    {
        /**
         * A buffer which can be used to read compressed data from CASC file.
         * When used with a stream, the stream will be able to transparently output decompressed data.
         */
        class ReadBuffer : public std::filebuf
        {
        private:
            static const uint32_t Signature = 0x45544C42;
            static const size_t BufferSize = 4096U;
            static const size_t DataHeaderSize = 30U;

            struct BufferInfo
            {
                typedef std::filebuf::traits_type Traits;

                // The offset of the first byte in the buffer.
                Traits::off_type begin;

                // The offset of the last byte in the buffer.
                Traits::off_type end;
            };

            struct ChunkInfo
            {
                typedef std::filebuf::traits_type Traits;

                // The offset of the first byte in the decompressed data.
                Traits::off_type begin;

                // The offset of the last byte in the decompressed data.
                Traits::off_type end;

                // The offset where the compressed data starts.
                // The base position is this->offset.
                Traits::off_type offset;

                // The size of the compressed data.
                size_t size;

                bool operator <(const ChunkInfo &b) const
                {
                    return this->begin < b.begin;
                }
                bool operator >(const ChunkInfo &b) const
                {
                    return this->begin > b.begin;
                }
            };

            // True when the file is properly initialized.
            // The file is properly initialized once all the headers have been read.
            bool isInitialized = false;

            // True if the file is currently buffering data.
            // When true, uflow will provide bytes from the actual file stream.
            // When false, uflow will provide bytes from the virtual file stream. 
            bool isBuffering = false;

            // The offset where the data starts.
            // This is where the first header is found.
            off_type offset = 0;

            // The size of the decompressed file.
            off_type length = 0;

            // The buffer containing the decompressed data for the current chunk.
            std::unique_ptr<char[]> out;

            // The buffer to use while buffering.
            std::unique_ptr<char[]> temp;

            // Currently buffered range
            BufferInfo currentBuffer;

            // Chunk handlers
            std::map<ChunkInfo, std::shared_ptr<Handler>> handlers;

            /**
             * Read the header for the current file, create handlers
             * and confirm checksums.
             */
            void init(std::string parameters)
            {
                using namespace Functions::Endian;
                using namespace Functions::Hash;

                EncodingMode encodingMode;
                std::vector<std::string> params;

                if (!parseParams(parameters, encodingMode, params) || encodingMode != EncodingMode::Blte)
                {
                    // TODO: Throw something?
                }

                // Read first header.
                char header[DataHeaderSize];
                std::filebuf::xsgetn(header, DataHeaderSize);

                std::array<uint8_t, 16> checksum;
                std::copy(header, header + 16, checksum.begin());
                std::reverse(checksum.begin(), checksum.end());
                auto size = read<EndianType::Little, uint32_t>(header + 0x10);

                // Checking the hash of the header.
                MD5 headerHasher;

                // Read BLTE header.
                char header2[8];

                uint32_t signature;

                std::filebuf::xsgetn(header2, 8);
                if ((signature = read<EndianType::Little, uint32_t>(header2)) != Signature)
                {
                    throw Exceptions::InvalidSignatureException(signature, 0x45544C42);
                }

                auto blockTableSize = read<EndianType::Big, uint32_t>(header2 + 0x04) - 8;

                // Add the first 8 bytes of the header.
                headerHasher.update(header2, 8);

                uint32_t bufferSize = 0;

                if (blockTableSize > 0)
                {
                    // Read rest of header.
                    auto bytes = std::make_unique<char[]>(blockTableSize);
                    std::filebuf::xsgetn(bytes.get(), blockTableSize);

                    // Update hasher with the rest of the header bytes.
                    headerHasher.update(bytes.get(), blockTableSize);

                    // Parse the header
                    auto pos = bytes.get();

                    auto tableMarker = read<EndianType::Big, uint8_t>(pos);
                    pos += sizeof(uint8_t);

                    if (tableMarker != 0x0F)
                    {
                        throw Exceptions::IOException("Invalid block table format.");
                    }

                    auto blockCount = read<EndianType::Big, uint8_t>(pos) << 16 | read<EndianType::Big, uint16_t>(pos + sizeof(uint8_t));
                    pos += sizeof(uint8_t) + sizeof(uint16_t);

                    uint32_t offset = 0;

                    for (int i = 0; i < blockCount; ++i)
                    {
                        auto compressedSize = read<EndianType::Big, uint32_t>(pos);
                        pos += sizeof(uint32_t);

                        auto decompressedSize = read<EndianType::Big, uint32_t>(pos);
                        pos += sizeof(uint32_t);

                        std::array<uint8_t, 16> checksum;
                        std::copy(pos, pos + 16, checksum.begin());
                        pos += 16;

                        ChunkInfo chunk
                        {
                            0,
                            decompressedSize,
                            38 + blockTableSize,
                            compressedSize
                        };

                        if (handlers.size() > 0)
                        {
                            const ChunkInfo &last = handlers.rbegin()->first;

                            chunk.begin = last.end;
                            chunk.end = chunk.begin + decompressedSize;
                            chunk.offset = last.offset + last.size;
                        }

                        auto buf = std::make_unique<char[]>(chunk.size);
                        auto pos = std::filebuf::seekoff(0, std::ios_base::cur, std::ios_base::in);
                        std::filebuf::seekoff(this->offset + chunk.offset, std::ios_base::beg, std::ios_base::in);
                        std::filebuf::xsgetn(buf.get(), chunk.size);

                        // Check the hash of the chunk.
                        MD5 chunkHasher;
                        chunkHasher.update(buf.get(), chunk.size);
                        chunkHasher.finalize();

                        if (chunkHasher.hexdigest() != Hex(checksum).string())
                        {
                            throw Exceptions::InvalidHashException(
                                lookup3(Hex(checksum).string(), 0),
                                lookup3(chunkHasher.hexdigest(), 0),
                                "");
                        }

                        if (handlers.find(chunk) == handlers.end())
                        {
                            handlers[chunk] = createHandler((EncodingMode)buf[0]);
                        }
                    }
                }
                else if (params.size() > 0)
                {
                    EncodingMode chunkMode;
                    std::vector<std::string> chunkParams;

                    if (!parseParams(params[0], chunkMode, chunkParams))
                    {
                        // TODO: Throw something?
                    }

                    ChunkInfo chunk
                    {
                        0,
                        size,
                        38,
                        size
                    };

                    auto blteSize = size_t(chunk.size - chunk.offset);

                    auto buf = std::make_unique<char[]>(blteSize);
                    auto pos = std::filebuf::seekoff(0, std::ios_base::cur, std::ios_base::in);
                    std::filebuf::seekoff(this->offset + chunk.offset, std::ios_base::beg, std::ios_base::in);
                    std::filebuf::xsgetn(buf.get(), blteSize);

                    headerHasher.update(buf.get(), blteSize);

                    if (handlers.find(chunk) == handlers.end())
                    {
                        handlers[chunk] = createHandler((EncodingMode)buf[0]);
                    }
                }
                else
                {
                    // TODO: Throw something?
                }

                // Finish the hash check of the header.
                headerHasher.finalize();

                Hex expected(checksum);

                if (headerHasher.hexdigest() != expected.string())
                {
                    throw Exceptions::InvalidHashException(
                        lookup3(expected.string(), 0),
                        lookup3(headerHasher.hexdigest(), 0),
                        "");
                }
            }

            /**
            * Finds the chunk that corresponds to the given virtual offset.
            */
            bool find(off_type offset, ChunkInfo &out)
            {
                for (auto &chunk : handlers)
                {
                    if (chunk.first.begin <= offset && chunk.first.end > offset)
                    {
                        out = chunk.first;
                        return true;
                    }
                }

                return false;
            }

            /**
            * The current position in the stream.
            */
            pos_type pos()
            {
                return pos_type(currentBuffer.begin + gptr() - eback());
            }

            /**
            * Seeks within the current buffer.
            */
            pos_type seekbuf(off_type offset, std::ios_base::seekdir dir = std::ios_base::cur)
            {
                switch (dir)
                {
                case std::ios_base::cur:
                    setg(eback(), gptr() + offset, egptr());
                    break;

                case std::ios_base::beg:
                    setg(eback(), eback() + offset, egptr());
                    break;

                case std::ios_base::end:
                    setg(eback(), egptr() - offset, egptr());
                    break;

                default:
                    throw Exceptions::IOException("Unsupported seek dir.");
                }

                return pos();
            }

            /**
            * Seeks relatively within the file.
            */
            pos_type seekrel(off_type offset, std::ios_base::seekdir dir = std::ios_base::cur)
            {
                if (dir != std::ios_base::cur || offset == 0)
                {
                    return pos();
                }

                if (offset <= showmanyc())
                {
                    return seekbuf(offset, dir);
                }

                return buffer(pos() + offset);
            }

            /**
            * Seeks absolutely within the file.
            */
            pos_type seekabs(off_type offset, std::ios_base::seekdir dir = std::ios_base::beg)
            {
                if (dir != std::ios_base::beg && dir != std::ios_base::end)
                {
                    return pos();
                }

                if (dir == std::ios_base::end)
                {
                    offset = length - offset;
                }

                return buffer(offset);
            }

            /**
            * Seeks within the file.
            */
            pos_type seek(off_type offset, std::ios_base::seekdir dir = std::ios_base::cur)
            {
                if (dir == std::ios_base::cur)
                {
                    return seekrel(offset, dir);
                }
                else
                {
                    return seekabs(offset, dir);
                }
            }

            /**
            * Read the decompressed data from the current chunk into the buffer.
            */
            pos_type buffer(off_type offset)
            {
                isBuffering = true;

                setg(temp.get(), temp.get(), temp.get() + BufferSize);

                // Use smaller than configured buffer if near end-of-file.
                size_t bufferSize = std::min<size_t>(BufferSize,
                    static_cast<size_t>(handlers.rbegin()->first.end - offset));

                pos_type end = offset + bufferSize;
                pos_type beg = offset;

                ChunkInfo chunk;
                size_t count;
                off_type pos;

                for (pos = 0; offset < end;)
                {
                    if (!find(offset, chunk))
                    {
                        return pos_type(-1);
                    }

                    count = std::min<size_t>(static_cast<size_t>(chunk.end - offset), bufferSize);

                    // Read the compression mode.
                    std::filebuf::seekpos(this->offset + chunk.offset, std::ios_base::in);
                    EncodingMode mode = (EncodingMode)std::filebuf::sbumpc();

                    if (handlers[chunk]->mode() != mode)
                    {
                        throw Casc::Exceptions::InvalidEncodingModeException(mode);
                    }

                    // Call the handler for the compression mode.
                    std::filebuf::seekpos(this->offset + chunk.offset + 1);
                    auto data = handlers[chunk]->decode(*this, offset - chunk.begin, chunk.size - 1, count);
                    std::memcpy(out.get() + pos, data.get(), count);

                    offset += count;
                    pos += count;
                    bufferSize -= count;
                }

                currentBuffer.begin = offset - pos;
                currentBuffer.end = offset;

                setg(out.get(), out.get(), out.get() + currentBuffer.end - currentBuffer.begin);

                isBuffering = false;

                return this->pos();
            }

            /**
             * Create the handler for an encoding mode.
             */
            std::shared_ptr<Handler> createHandler(EncodingMode mode) const
            {
                switch (mode)
                {
                case None:
                    return std::shared_ptr<Handler>(new Impl::DefaultHandler());

                case Zlib:
                    return std::shared_ptr<Handler>(new Impl::ZlibHandler());

                default:
                    throw Exceptions::InvalidEncodingModeException(mode);
                }
            }

            /**
            * Checks if a value is whitespace.
            */
            inline bool isWhitespace(int value) const
            {
                bool result;

                result = value == ' ' || value == '\t' || value == '\v' || value == '\r' || value == '\f' || value == '\n';
                return result;
            }

            /**
            * Parse params.
            */
            int parseParams(char *encodingProfile, char **encodingMode, char ***params, unsigned int *nParams) const
            {
                char *it;

                for (it = encodingProfile; ; ++it)
                {
                    if (!isWhitespace(*it))
                        break;
                }

                for (*encodingMode = it; *it; ++it)
                {
                    if (*it == ':' || isWhitespace(*it))
                        break;
                }

                char *encodingModeEnd = it;

                while (isWhitespace(*it))
                {
                    ++it;
                }

                if (*it != '\0')
                {
                    if (*it != ':')
                        return 0;

                    *encodingModeEnd = '\0';

                    do
                    {
                        ++it;
                    } while (isWhitespace(*it));

                    if (*it == '{')
                    {
                        do
                        {
                            ++it;
                        } while (isWhitespace(*it));

                        *nParams = 1;

                        if (!*it)
                            return 0;

                        char *paramsStart = it;

                        int state = 1;
                        do
                        {
                            if (!isWhitespace(*it))
                            {
                                if (!state)
                                    return 0;

                                if (*it == '{')
                                {
                                    state++;
                                }
                                else
                                {
                                    if (*it == '}')
                                    {
                                        --state;
                                    }
                                    else
                                    {
                                        if (*it == ',' && state == 1)
                                            ++*nParams;
                                    }
                                }
                            }
                        } while (*++it);

                        if (state)
                            return 0;

                        char **paramArray = (char **)new char[4 * (*nParams) | -((uint64_t)*nParams >> 30 != 0)];

                        if (*params)
                            delete [] *params;
                        *params = paramArray;

                        int paramIndex = 0;
                        **params = paramsStart;
                        for (int state = 1; *paramsStart; ++paramsStart)
                        {
                            if (*paramsStart != ' ' && *paramsStart != '\t' && *paramsStart != '\v' && *paramsStart != '\r' && *paramsStart != '\f' && *paramsStart != '\n')
                            {
                                if (!state)
                                    return 0;

                                if (*paramsStart == '{')
                                {
                                    ++state;
                                }
                                else if (*paramsStart == '}')
                                {
                                    --state;

                                    if (!state)
                                    {
                                        char *paramsEnd;
                                        for (paramsEnd = paramsStart; paramsEnd > (*params)[paramIndex]; --paramsEnd)
                                        {
                                            char ch = *(paramsEnd - 1);
                                            if (ch != ' ' && ch != '\t' && ch != '\v' && ch != '\r' && ch != '\f' && ch != '\n')
                                                break;
                                        }
                                        *paramsEnd = '\0';
                                    }
                                }
                                else if (*paramsStart == ',' && state == 1)
                                {
                                    char *paramsEnd = paramsStart;
                                    if (paramsStart > (*params)[paramIndex])
                                    {
                                        do
                                        {
                                            if (!isWhitespace(*(paramsEnd - 1)))
                                                break;
                                            --paramsEnd;
                                        } while (paramsEnd > (*params)[paramIndex]);
                                    }
                                    *paramsEnd = '\0';
                                    ++paramIndex;
                                    for (; *paramsStart; ++paramsStart)
                                    {
                                        if (!isWhitespace(paramsStart[1]))
                                            break;
                                    }
                                    (*params)[paramIndex] = paramsStart + 1;
                                }
                            }
                        }

                        if (paramIndex + 1 != *nParams)
                            abort();
                    }
                    else
                    {
                        *nParams = 1;

                        char **paramArray = (char **)new char[4];

                        if (*params)
                            delete [] *params;
                        *params = paramArray;

                        **params = it;

                        for (it = &it[strlen(it)]; it > **params; --it)
                        {
                            char ch = *(it - 1);
                            if (ch != ' ' && ch != '\t' && ch != '\v' && ch != '\r' && ch != '\f' && ch != '\n')
                                break;
                        }

                        *it = '\0';
                    }

                    if (*nParams == 1 && !***params)
                    {
                        *nParams = 0;

                        if (*params)
                            delete [] *params;
                        *params = 0;
                    }
                }
                return 1;
            }
            
            /**
            * Parse params.
            */
            bool parseParams(std::string input, EncodingMode &mode, std::vector<std::string> &chunks) const
            {
                std::vector<char> temp(input.size() + 1, '\0');
                std::memcpy(temp.data(), input.data(), input.size() + 1);

                char *encodingMode = nullptr;
                char **params = nullptr;
                unsigned int nParams = 0;

                if (!parseParams(temp.data(), &encodingMode, &params, &nParams))
                {
                    return false;
                }

                mode = (EncodingMode)*encodingMode;

                for (auto i = 0U; i < nParams; ++i)
                {
                    chunks.push_back(params[i]);
                }

                return true;
            }

        protected:
            pos_type seekpos(pos_type pos,
                std::ios_base::openmode which = std::ios_base::in) override
            {
                if (isBuffering)
                {
                    return std::filebuf::seekpos(pos, which);
                }

                return seekoff(pos, std::ios_base::beg, which);
            }

            pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                std::ios_base::openmode which = std::ios_base::in) override
            {
                if (isBuffering)
                {
                    return std::filebuf::seekoff(off, dir, which);
                }

                return seek(off, dir);
            }

            std::streamsize showmanyc() override
            {
                return egptr() - gptr();
            }

            int_type underflow() override
            {
                if (isInitialized && !isBuffering)
                {
                    if (seek(0) == pos_type(-1) || seek(0) == this->length)
                    {
                        setg(nullptr, nullptr, nullptr);
                        return traits_type::eof();
                    }

                    buffer(pos());
                    return traits_type::to_int_type(out.get()[0]);
                }

                return std::filebuf::underflow();
            }

            int_type uflow() override
            {
                if (isInitialized && !isBuffering)
                {
                    if (seek(0) == pos_type(-1) || seek(0) == this->length)
                    {
                        setg(nullptr, nullptr, nullptr);
                        return traits_type::eof();
                    }

                    seekbuf(1);

                    return traits_type::to_int_type(out.get()[0]);
                }

                return std::filebuf::uflow();
            }

            std::streambuf *setbuf(char_type *s, std::streamsize n) override
            {
                return this;
            }

            std::streamsize xsgetn(char_type* s, std::streamsize count) override
            {
                if (isBuffering)
                {
                    return std::filebuf::xsgetn(s, count);
                }

                std::streamsize copied = 0;

                if (count <= showmanyc())
                {
                    std::memcpy(s, gptr(), static_cast<size_t>(count));
                    seekbuf(count);
                    copied = count;
                }
                else
                {
                    do
                    {
                        if (showmanyc() == 0)
                        {
                            buffer(pos());
                        }

                        auto numRead = std::min<size_t>(
                            static_cast<size_t>(showmanyc()), static_cast<size_t>(count - copied));
                        std::memcpy(s + copied, gptr(), numRead);
                        copied += numRead;
                        seekbuf(numRead);
                    } while (copied < count && underflow() != traits_type::eof());
                }

                return copied;
            }

        public:
            /**
             * Default constructor.
             */
            ReadBuffer() :
                out(std::make_unique<char[]>(BufferSize)),
                temp(std::make_unique<char[]>(BufferSize))
            {
            }

            /**
            * Constructor.
            */
            ReadBuffer(std::string parameters) :
                out(std::make_unique<char[]>(BufferSize)),
                temp(std::make_unique<char[]>(BufferSize))
            {
            }

            /**
             * Move constructor.
             */
            ReadBuffer(ReadBuffer &&) = default;

            /**
             * Move operator.
             */
            ReadBuffer &operator= (ReadBuffer &&) = default;

            /**
             * Destructor.
             */
            virtual ~ReadBuffer() = default;

            /**
             * Opens a file from the currently opened CASC file.
             */
            void open(size_t offset, std::string parameters)
            {
                this->isInitialized = false;

                if (!is_open())
                {
                    throw Exceptions::IOException("Buffer is not open.");
                }

                handlers.clear();

                this->offset = offset;
                std::filebuf::seekpos(offset);

                this->init(parameters);

                this->currentBuffer.begin = 0;
                this->currentBuffer.end = -1;
                this->setg(nullptr, nullptr, nullptr);

                if (handlers.size() > 0)
                {
                    this->length = static_cast<size_t>(handlers.rbegin()->first.end);
                    seek(0);
                }

                this->isInitialized = true;
            }

            /**
             * Opens a file in a CASC file.
             */
            void open(const std::string filename, size_t offset)
            {
                if (std::filebuf::open(filename, std::ios_base::in | std::ios_base::binary) == nullptr)
                {
                    throw Exceptions::IOException("Couldn't open buffer.");
                }

                open(offset, "");
            }

            /**
             * Opens a file in a CASC file.
             */
            void open(const std::string filename, size_t offset, std::string parameters)
            {
                if (std::filebuf::open(filename, std::ios_base::in | std::ios_base::binary) == nullptr)
                {
                    throw Exceptions::IOException("Couldn't open buffer.");
                }

                open(offset, parameters);
            }

            /**
             * Closes the currently opened CASC file.
             */
            void close()
            {
                this->setg(nullptr, nullptr, nullptr);

                if (std::filebuf::is_open())
                {
                    std::filebuf::close();
                }

                isInitialized = false;
            }
        };
    }
}