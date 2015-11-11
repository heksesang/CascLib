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

#include "../Exceptions.hpp"

#include "../md5.hpp"
#include "../zlib.hpp"

#include "Handler.hpp"
#include "Endian.hpp"
#include "../Hex.hpp"

namespace Casc
{
    namespace IO
    {
        /**
         * 
         */
        class Buffer : public std::streambuf
        {
        private:
            static const uint32_t Signature = 0x45544C42;
            static const size_t DataHeaderSize = 30U;
            static const size_t BufferSize = 4096U;

            // The underlying stream buffer.
            std::shared_ptr<std::fstream> fbuf;

            // True when the file is properly initialized.
            // The file is properly initialized once all the headers have been read.
            bool isInitialized = false;

            // The logical size of the file.
            size_t length;

            // The offset of the file.
            size_t offset;

            // The offset of the buffer.
            size_t current;

            // The buffer.
            std::vector<char> buf;

            // Chunk handlers.
            std::vector<std::shared_ptr<Handler>> handlers;

            /**
             * Read the header for the current file, create handlers
             * and confirm checksums.
             */
            void init()
            {
                handlers.clear();
                length = 0;
                current = 0;

                std::array<char, DataHeaderSize> dataHeader;
                fbuf->read(dataHeader.data(), DataHeaderSize);
                this->offset += 30;

                std::array<uint8_t, 16> blockTableChecksum;
                std::copy(dataHeader.data(), dataHeader.data() + 16, blockTableChecksum.begin());
                std::reverse(blockTableChecksum.begin(), blockTableChecksum.end());
                auto size = Endian::read<EndianType::Little, uint32_t>(dataHeader.data() + 16);

                MD5 blockTableVerficiation;

                std::array<char, 8> header;
                fbuf->read(header.data(), header.size());
                this->offset += 8;

                blockTableVerficiation.update(header.data(), header.size());

                auto blockTableSize = getBlockTableSize(header.begin());
                this->offset += blockTableSize;

                if (blockTableSize > 0)
                {
                    std::vector<char> blockTable(blockTableSize);
                    fbuf->read(blockTable.data(), blockTableSize);

                    blockTableVerficiation.update(blockTable);
                    blockTableVerficiation.finalize();

                    // TODO: Compare checksums

                    auto chunks = parseBlockTable(blockTable.begin(), blockTable.end());

                    for (auto &chunk : chunks)
                    {
                        fbuf->seekg(this->offset + chunk.offset);
                        EncodingMode mode = (EncodingMode)fbuf->get();

                        auto source = std::make_shared<Impl::StreamSource>(fbuf, std::make_pair(this->offset + chunk.offset, this->offset + chunk.offset + chunk.size));

                        handlers.push_back(createHandler(mode, chunk, source));
                    }
                }

                for (auto &handler : handlers)
                {
                    length += handler->logicalSize();
                }
            }

            /**
             * The current position in the stream.
             */
            pos_type pos()
            {
                return pos_type(current + gptr() - eback());
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

                auto newOffset = pos() + offset;

                if (newOffset >= current && newOffset < current + 4096)
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

                if (offset >= current && offset < current + 4096)
                {
                    return seekbuf(offset - current, std::ios_base::beg);
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
                auto count = 0U;

                for (auto &handler : handlers)
                {
                    if (count < BufferSize && ((handler->chunk.begin < offset
                        && handler->chunk.end > offset) || handler->chunk.begin >= offset))
                    {
                        auto begin = (handler->chunk.begin < offset
                            && handler->chunk.end > offset) ? size_t(offset - handler->chunk.begin) : 0;
                        auto decoded = handler->decode(begin, BufferSize - count);

                        std::memcpy(buf.data() + count, decoded.data(), decoded.size());
                        count += decoded.size();
                    }
                }

                if (count < BufferSize)
                {
                    std::memset(buf.data() + count, 0, BufferSize - count);
                }

                setg(buf.data(), buf.data(), buf.data() + count);

                current = size_t(offset);

                return pos();
            }

        protected:
            pos_type seekpos(pos_type pos,
                std::ios_base::openmode which = std::ios_base::in) override
            {
                return seekoff(pos, std::ios_base::beg, which);
            }

            pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                std::ios_base::openmode which = std::ios_base::in) override
            {
                return seek(off, dir);
            }

            std::streamsize showmanyc() override
            {
                return egptr() - gptr();
            }

            int_type underflow() override
            {
                if (pos() == pos_type(length))
                {
                    setg(nullptr, nullptr, nullptr);
                    return traits_type::eof();
                }

                buffer(pos());
                return traits_type::to_int_type(buf[0]);
            }

            int_type uflow() override
            {
                if (pos() == pos_type(length))
                {
                    setg(nullptr, nullptr, nullptr);
                    return traits_type::eof();
                }

                buffer(pos());
                seekbuf(1);

                return traits_type::to_int_type(buf[1]);
            }

            std::streamsize xsgetn(char_type* s, std::streamsize count) override
            {
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

            std::streambuf *setbuf(char_type *s, std::streamsize n) override
            {
                return this;
            }

        public:
            /**
             * Default constructor.
             */
            Buffer()
                : buf(BufferSize), fbuf(new std::fstream())
            {
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
            virtual ~Buffer() = default;

            /**
             * Reads a file from a new offset within the open data file.
             * Throws if is_open() is false.
             */
            void open(size_t offset)
            {
                this->isInitialized = false;

                if (!fbuf->is_open())
                {
                    throw Exceptions::IOException("Buffer is not open.");
                }

                this->offset = offset;
                fbuf->seekg(offset);

                this->init();

                this->isInitialized = true;
            }

            /**
             * Opens a data file and reads a file from an offset.
             */
            void open(const std::string filename, size_t offset)
            {
                fbuf->open(filename, std::ios_base::in | std::ios_base::binary);
                
                if (fbuf->fail())
                {
                    throw Exceptions::IOException("Couldn't open buffer.");
                }

                open(offset);
            }

            /**
             * Checks if the buffer is open.
             */
            bool is_open() const
            {
                return fbuf->is_open();
            }

            /**
             * Closes the stream.
             */
            void close()
            {
                setg(nullptr, nullptr, nullptr);

                if (fbuf->is_open())
                {
                    fbuf->close();
                }

                isInitialized = false;
            }

            /**
            * Checks if the file has a block table.
            */
            template <typename InputIt>
            static size_t getBlockTableSize(InputIt begin)
            {
                uint32_t signature;

                if ((signature = Endian::read<EndianType::Little, uint32_t>(begin)) != Signature)
                {
                    throw Exceptions::InvalidSignatureException(signature, 0x45544C42);
                }

                return std::max(Endian::read<EndianType::Big, uint32_t>(begin + 4) - 8, 0U);
            }

            /**
            * Parses the block table.
            */
            template <typename InputIt>
            static std::vector<Chunk> parseBlockTable(InputIt begin, InputIt end)
            {
                auto tableMarker = Endian::read<EndianType::Big, uint8_t>(begin);

                if (tableMarker != 0x0F)
                {
                    throw Exceptions::IOException("Invalid block table format.");
                }

                auto blockCount = Endian::read<EndianType::Big, uint8_t>(begin + 1) << 16 |
                    Endian::read<EndianType::Big, uint16_t>(begin + 1 + sizeof(uint8_t));

                std::vector<Chunk> chunks;

                for (auto it = begin + 4; it < end; it += 24)
                {
                    auto physicalSize = Endian::read<EndianType::Big, uint32_t>(it);
                    auto logicalSize = Endian::read<EndianType::Big, uint32_t>(it + 4);
                    std::array<char, 16> checksumBytes;
                    std::copy(it + 8, it + 24, checksumBytes.data());

                    chunks.push_back({
                        chunks.size() > 0 ? chunks.rbegin()->end : 0,
                        chunks.size() > 0 ? chunks.rbegin()->end + logicalSize : logicalSize,
                        chunks.size() > 0 ? chunks.rbegin()->offset + chunks.rbegin()->size : 0,
                        physicalSize,
                        checksumBytes
                    });
                }

                return chunks;
            }

            /**
             * Create the handler for an encoding mode.
             */
            static std::shared_ptr<Handler> createHandler(EncodingMode mode, Chunk chunk, std::shared_ptr<DataSource> source)
            {
                switch (mode)
                {
                case EncodingMode::None:
                    return std::make_shared<Impl::NoneHandler>(chunk, source);

                case EncodingMode::Zlib:
                    return std::make_shared<Impl::ZlibHandler>(chunk, source);

                case EncodingMode::Crypt:
                    return std::make_shared<Impl::CryptHandler>(chunk, source);

                default:
                    throw Exceptions::InvalidEncodingModeException(mode);
                }
            }
        };
    }
}