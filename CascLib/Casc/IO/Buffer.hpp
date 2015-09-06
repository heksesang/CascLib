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
        using namespace Casc::Shared;

        /**
         * A buffer which can be used to read compressed data from CASC file.
         * When used with a stream, the stream will be able to transparently output decompressed data.
         */
        class Buffer : public std::filebuf
        {
        private:
            const uint32_t BlteSignature = 0x45544C42;
            const size_t BufferSize = 4096U;

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

                // Data is compressed.
                /*bool compressed;*/
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

            // The available chunks.
            std::vector<ChunkInfo> chunks;

            // Chunk handlers
            std::map<char, std::shared_ptr<Handler>> handlers;

            /**
             * Read the header for the current file.
             * This sets up all necessary chunk data for directly reading decompressed data.
             */
            void readHeader()
            {
                using namespace Functions::Endian;
                using namespace Functions::Hash;

                // Read first header.
                char header[0x1E];
                std::filebuf::xsgetn(header, 0x1E);

                std::array<uint8_t, 16> checksum;
                std::copy(header, header + 16, checksum.begin());
                std::reverse(checksum.begin(), checksum.end());
                auto size = read<IO::EndianType::Little, uint32_t>(header + 0x10);

                // Checking the hash of the header.
                MD5 headerHasher;

                // Read BLTE header.
                char header2[0x08];

                uint32_t signature;

                std::filebuf::xsgetn(header2, 0x08);
                if ((signature = read<IO::EndianType::Little, uint32_t>(header2)) != BlteSignature)
                {
                    throw Exceptions::InvalidSignatureException(signature, 0x45544C42);
                }

                auto readBytes = read<IO::EndianType::Big, uint32_t>(header2 + 0x04);

                // Add the first 8 bytes of the header.
                headerHasher.update(header2, 8);

                uint32_t bufferSize = 0;

                if (readBytes > 0)
                {
                    readBytes -= 0x08;

                    // Read rest of header.
                    auto bytes = std::make_unique<char[]>(readBytes);
                    std::filebuf::xsgetn(bytes.get(), readBytes);

                    // Update hasher with the rest of the header bytes.
                    headerHasher.update(bytes.get(), readBytes);

                    // Parse the header
                    auto pos = bytes.get();

                    auto flags = read<IO::EndianType::Big, uint16_t>(pos);
                    pos += sizeof(uint16_t);
                    auto chunkCount = read<IO::EndianType::Big, uint16_t>(pos);
                    pos += sizeof(uint16_t);

                    for (int i = 0; i < chunkCount; ++i)
                    {
                        auto compressedSize = read<IO::EndianType::Big, uint32_t>(pos);
                        pos += sizeof(uint32_t);

                        auto decompressedSize = read<IO::EndianType::Big, uint32_t>(pos);
                        pos += sizeof(uint32_t);

                        std::array<uint8_t, 16> checksum;
                        std::copy(pos, pos + 16, checksum.begin());
                        pos += 16;

                        ChunkInfo chunk
                        {
                            0,
                            decompressedSize,
                            38 + readBytes,
                            compressedSize
                        };

                        if (chunks.size() > 0)
                        {
                            auto last = chunks.back();

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

                        chunks.push_back(chunk);
                    }
                }
                else
                {
                    chunks.push_back({
                        0,
                        size,
                        38,
                        size
                    });

                    ChunkInfo &chunk = chunks.back();
                    auto blteSize = size_t(chunk.size - chunk.offset);

                    auto buf = std::make_unique<char[]>(blteSize);
                    auto pos = std::filebuf::seekoff(0, std::ios_base::cur, std::ios_base::in);
                    std::filebuf::seekoff(this->offset + chunk.offset, std::ios_base::beg, std::ios_base::in);
                    std::filebuf::xsgetn(buf.get(), blteSize);

                    headerHasher.update(buf.get(), blteSize);
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
            *
            * @param offset	the virtual offset of the decompressed data.
            * @param out		reference to the output variable where the chunk will be written.
            * @return			true on success, false on failure.
            */
            bool find(off_type offset, ChunkInfo &out)
            {
                for (auto &chunk : chunks)
                {
                    if (chunk.begin <= offset && chunk.end > offset)
                    {
                        out = chunk;
                        return true;
                    }
                    else if (chunk.begin <= offset && chunk.end >= offset &&
                        chunks.back().begin == chunk.begin && chunks.back().end == chunk.end)
                    {
                        out = chunk;
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
            *
            * @param offset the offset to seek to.
            * @param dir    the seek direction.
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
            *
            * @param offset the offset to seek to.
            * @param dir    the seek direction.
            */
            pos_type seekrel(off_type offset, std::ios_base::seekdir dir = std::ios_base::cur)
            {
                if (dir != std::ios_base::cur)
                {
                    return pos();
                }

                return buffer(pos() + offset);
            }

            /**
            * Seeks absolutely within the file.
            *
            * @param offset the offset to seek to.
            * @param dir    the seek direction.
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
            *
            * @param offset the offset to seek to.
            * @param dir    the seek direction.
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
            *
            * @param offset    the offset into the chunk to start reading.
            */
            pos_type buffer(off_type offset)
            {
                isBuffering = true;

                setg(temp.get(), temp.get(), temp.get() + BufferSize);

                // Use smaller than configured buffer if near end-of-file.
                size_t bufferSize = std::min<size_t>(BufferSize,
                    static_cast<size_t>(chunks.back().end - offset));

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

                    off_type chunkSize = 0;

                    count = std::min<size_t>(static_cast<size_t>(chunk.end - offset), bufferSize);

                    // Read the compression mode.
                    std::filebuf::seekpos(this->offset + chunk.offset, std::ios_base::in);
                    EncodingMode mode = (EncodingMode)std::filebuf::sbumpc();

                    // Call the handler for the compression mode.
                    std::filebuf::seekpos(this->offset + chunk.offset + 1);
                    auto data = handlers[mode]->read(*this, offset - chunk.begin, chunk.size - 1, count, chunkSize);
                    std::memcpy(out.get() + pos, data.get(), count);

                    if (chunks.size() == 1 && chunkSize != 0)
                    {
                        this->length = chunks.back().end = chunkSize;
                    }

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
                    if (seek(0) == pos_type(-1))
                    {
                        setg(nullptr, nullptr, nullptr);
                        return traits_type::eof();
                    }

                    return traits_type::to_int_type(out.get()[0]);
                }

                return std::filebuf::underflow();
            }

            int_type uflow() override
            {
                if (isInitialized && !isBuffering)
                {
                    if (seek(0) == pos_type(-1))
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
                        auto numRead = std::min<size_t>(
                            static_cast<size_t>(showmanyc()), static_cast<size_t>(count - copied));
                        std::memcpy(s + copied, gptr(), numRead);
                        copied += numRead;
                        numRead == 0 ? seek(numRead) : seekbuf(numRead);
                    } while (copied < count && underflow() != traits_type::eof());
                }

                return copied;
            }

        public:
            /**
             * Default constructor.
             */
            Buffer()
                : out(std::make_unique<char[]>(BufferSize)),
                temp(std::make_unique<char[]>(BufferSize))
            {
                registerHandler<DefaultHandler>();
            }

            /**
             * Constructor with handler initialization.
             */
            Buffer(std::vector<std::shared_ptr<Handler>> handlers)
                : Buffer()
            {
                registerHandlers(handlers);
            }

            /**
             * Move constructor.
             */
            Buffer(Buffer &&) = default;

            /**
             * Move operator.
             */
            Buffer &operator= (Buffer &&) = default;

            /**
             * Destructor.
             */
            virtual ~Buffer() override
            {
                this->close();
            }

            void registerHandlers(std::vector<std::shared_ptr<Handler>> handlers)
            {
                for (std::shared_ptr<Handler> handler : handlers)
                    this->handlers[handler->mode()] = std::shared_ptr<Handler>(handler);
            }

            template <typename T>
            void registerHandler()
            {
                Handler* handler = new T;
                this->handlers[handler->mode()] = std::shared_ptr<Handler>(handler);
            }

            /**
             * Opens a file from the currently opened CASC file.
             *
             * @param offset	the offset where the file starts.
             */
            void open(size_t offset)
            {
                this->isInitialized = false;

                if (!is_open())
                {
                    throw Exceptions::IOException("Buffer is not open.");
                }

                chunks.clear();

                this->offset = offset;
                std::filebuf::seekpos(offset);

                this->readHeader();

                this->currentBuffer.begin = 0;
                this->currentBuffer.end = -1;
                this->setg(nullptr, nullptr, nullptr);

                if (chunks.size() > 0)
                {
                    this->length = static_cast<size_t>(chunks.back().end);
                    seek(0);
                }

                this->isInitialized = true;
            }

            /**
             * Opens a file in a CASC file.
             *
             * @param filename	the filename of the CASC file.
             * @param offset	the offset where the file starts.
             */
            void open(const std::string filename, size_t offset)
            {
                if (std::filebuf::open(filename, std::ios_base::in | std::ios_base::binary) == nullptr)
                {
                    throw Exceptions::IOException("Couldn't open buffer.");
                }

                open(offset);
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