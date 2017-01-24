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
             * Set default values for counts and clear collections.
             */
            void setDefaults()
            {
                handlers.clear();
                length = 0;
                current = 0;
            }

            /**
             * Read the data header (only present in local CASC archives).
             */
            std::array<char, DataHeaderSize> readDataHeader()
            {
                std::array<char, DataHeaderSize> dataHeader;
                this->fbuf->read(dataHeader.data(), DataHeaderSize);
                this->offset += 30;

                return std::move(dataHeader);
            }

            template <typename T, size_t Size>
            std::string arrayToHexString(const std::array<T, Size>& arr) const
            {
                std::stringstream ss;
                for (const T& byte : arr)
                {
                    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
                }

                return ss.str();
            }

            /**
             * Read the checksum of the header (BLTE).
             */
            std::string getChecksum(const std::array<char, DataHeaderSize>& dataHeader) const
            {
                std::array<uint8_t, 16> checksum;
                std::copy(dataHeader.data(), dataHeader.data() + 16, checksum.begin());
                std::reverse(checksum.begin(), checksum.end());

                return arrayToHexString(checksum);
            }

            /**
             * Read the size of the file from data header.
             */
            size_t getFileSize(const std::array<char, DataHeaderSize>& dataHeader) const
            {
                return static_cast<size_t>(Endian::read<EndianType::Little, uint32_t>(dataHeader.data() + 16));
            }

            /**
             * Read the header (BLTE).
             */
            std::array<char, 8> readHeader()
            {
                std::array<char, 8> header;
                fbuf->read(header.data(), header.size());
                this->offset += 8;

                return std::move(header);
            }

            /**
             * Checks the signature of the file.
             */
            template <typename InputIt>
            static void checkSignature(InputIt begin)
            {
                uint32_t signature;

                if ((signature = Endian::read<EndianType::Little, uint32_t>(begin)) != Signature)
                {
                    throw Exceptions::InvalidSignatureException(signature, 0x45544C42);
                }
            }

            /**
             * Calculate the total logical size of the file.
             */
            void calculateLogicalStreamLength()
            {
                for (auto &handler : handlers)
                {
                    this->length += handler->logicalSize();
                }
            }

            /**
             * Handle parsing of a file without block table.
             */
            void handleMissingBlockTable(size_t fileSize)
            {
                auto offset = fbuf->tellg();

                if (offset < 0)
                {
                    throw Exceptions::IOException("Negative stream offset.");
                }

                EncodingMode mode = (EncodingMode)fbuf->get();
                auto source = std::make_shared<Impl::StreamSource>(fbuf, std::make_pair(size_t(offset), size_t(offset) + fileSize));

                handlers.push_back(createHandler(mode, source));
            }

            /**
             * Create handlers for all the chunks.
             */
            void createHandlers(std::vector<Chunk> chunks)
            {
                for (auto &chunk : chunks)
                {
                    fbuf->seekg(this->offset + chunk.offset);
                    EncodingMode mode = (EncodingMode)fbuf->get();

                    auto source = std::make_shared<Impl::StreamSource>(fbuf, std::make_pair(this->offset + chunk.offset, this->offset + chunk.offset + chunk.size));

                    handlers.push_back(createHandler(mode, chunk, source));
                }
            }

            bool isArchive(const std::string& filename) const
            {
                return std::experimental::filesystem::path(filename).has_extension();
            }

            size_t getFileSize(const std::string& filename) const
            {
                return static_cast<size_t>(std::experimental::filesystem::file_size(filename));
            }

            /**
             * Read the header for the current file, create handlers and confirm checksums.
             */
            void init(std::string filename)
            {
                setDefaults();

                auto headerChecksum = filename;
                auto fileSize = getFileSize(filename);

                if (isArchive(filename))
                {
                    auto dataHeader = readDataHeader();
                    headerChecksum = getChecksum(dataHeader);
                    fileSize = getFileSize(dataHeader);
                }

                MD5 calculatedHeaderChecksum;

                auto header = readHeader();
                calculatedHeaderChecksum.update(header.data(), static_cast<MD5::size_type>(header.size()));

                checkSignature(header.begin());

                auto blockTableSize = getBlockTableSize(header.begin());

                if (blockTableSize > 0)
                {
                    std::vector<char> blockTable(blockTableSize);
                    fbuf->read(blockTable.data(), blockTableSize);

                    calculatedHeaderChecksum.update(blockTable);
                    calculatedHeaderChecksum.finalize();

                    auto actualHeaderChecksum = calculatedHeaderChecksum.hexdigest();

                    if (actualHeaderChecksum != headerChecksum)
                    {
                        throw Exceptions::InvalidHashException<std::string>(headerChecksum, actualHeaderChecksum, filename);
                    }

                    createHandlers(createChunks(blockTable.cbegin(), blockTable.cend()));
                }
                else
                {
                    handleMissingBlockTable(fileSize);
                }

                calculateLogicalStreamLength();
            }

            /**
             * The current position in the stream.
             */
            size_t pos()
            {
                return size_t(current + gptr() - eback());
            }

            /**
             * Seeks within the current buffer.
             */
            size_t seekbuf(off_type offset, std::ios_base::seekdir dir = std::ios_base::cur)
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
            size_t seekrel(off_type offset, std::ios_base::seekdir dir = std::ios_base::cur)
            {
                if (dir != std::ios_base::cur || offset == 0 || (pos() + offset) >= length || (pos() + offset) < 0)
                {
                    return pos();
                }

                size_t position = pos() + static_cast<size_t>(offset);

                if (position != traits_type::eof()) {

                    if (position >= current && position < current + 4096)
                    {
                        return seekbuf(offset, dir);
                    }
                }

                return buffer(position);
            }

            /**
             * Seeks absolutely within the file.
             */
            size_t seekabs(size_t offset, std::ios_base::seekdir dir = std::ios_base::beg)
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
            size_t seek(off_type offset, std::ios_base::seekdir dir = std::ios_base::cur)
            {
                if (dir == std::ios_base::cur)
                {
                    return seekrel(offset, dir);
                }
                else
                {
                    return seekabs(static_cast<size_t>(offset), dir);
                }
            }

            /**
             * Read the decompressed data from the current chunk into the buffer.
             */
            size_t buffer(size_t offset)
            {
                size_t count = 0U;

                for (auto &handler : handlers)
                {
                    if (count < BufferSize && (
                        (handler->chunk.begin < offset && handler->chunk.end > offset) ||
                         handler->chunk.begin >= offset))
                    {
                        auto begin = (handler->chunk.begin < offset
                            && handler->chunk.end > offset) ? offset - handler->chunk.begin : 0;
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
                        if (showmanyc() == 0 && pos() != pos_type(length))
                        {
                            buffer(pos());
                        }

                        auto numRead = std::min(
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
             * Initializes the buffer for reading a new file.
             * Throws if is_open() is false.
             */
            void initialize(std::string filename, size_t offset)
            {
                this->isInitialized = false;

                if (!fbuf->is_open())
                {
                    throw Exceptions::IOException("Buffer is not open.");
                }

                this->offset = offset;
                fbuf->seekg(offset);

                this->init(filename);

                this->isInitialized = true;
            }

            /**
             * Opens a file and starts initialization from offset.
             */
            void open(const std::string filename, size_t offset)
            {
                if (fbuf->is_open()) {
                    fbuf->close();
                }

                fbuf->open(filename, std::ios_base::in | std::ios_base::binary);

                if (fbuf->fail())
                {
                    throw Exceptions::IOException("Couldn't open buffer.");
                }

                initialize(filename, offset);
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
            size_t getBlockTableSize(InputIt begin)
            {
                auto size = Endian::read<EndianType::Big, uint32_t>(begin + 4);
                size = size > 0 ? size - 8 : size;
                this->offset += size;

                return size;
            }

            /**
             * Parses the block table.
             */
            template <typename InputIt>
            static std::vector<Chunk> createChunks(InputIt beginBlockTable, InputIt endBlockTable)
            {
                auto tableMarker = Endian::read<EndianType::Big, uint8_t>(beginBlockTable);

                if (tableMarker != 0x0F)
                {
                    throw Exceptions::IOException("Invalid block table format.");
                }

                auto blockCount = Endian::read<EndianType::Big, uint8_t>(beginBlockTable + 1) << 16 |
                    Endian::read<EndianType::Big, uint16_t>(beginBlockTable + 1 + sizeof(uint8_t));

                std::vector<Chunk> chunks;

                for (auto it = beginBlockTable + 4; it < endBlockTable; it += 24)
                {
                    auto physicalSize = Endian::read<EndianType::Big, uint32_t>(it);
                    auto logicalSize = Endian::read<EndianType::Big, uint32_t>(it + 4);
                    std::array<char, 16> checksumBytes;
                    std::copy(it + 8, it + 24, checksumBytes.begin());

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

            /**
             * Create the handler for an encoding mode.
             */
            static std::shared_ptr<Handler> createHandler(EncodingMode mode, std::shared_ptr<DataSource> source)
            {
                switch (mode)
                {
                case EncodingMode::None:
                    return std::make_shared<Impl::NoneHandler>(source);

                case EncodingMode::Zlib:
                    return std::make_shared<Impl::ZlibHandler>(source);

                case EncodingMode::Crypt:
                    return std::make_shared<Impl::CryptHandler>(source);

                default:
                    throw Exceptions::InvalidEncodingModeException(mode);
                }
            }
        };
    }
}