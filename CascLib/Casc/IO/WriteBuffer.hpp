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

#include <cstring>
#include <fstream>
#include <streambuf>
#include <vector>

#include "../Common.hpp"

#include "../Parsers/Binary/ShadowMemory.hpp"
#include "StreamAllocator.hpp"

namespace Casc
{
    namespace IO
    {
        class WriteBuffer : public std::filebuf
        {
            static const int Signature = 0x45544C42;

            // The path of the data folder.
            std::string basePath;

            // The current block count.
            size_t blockCount;

            // The encoded data.
            std::vector<char> blockTable;

            // The encoded data.
            std::vector<char> encoded;

            // The buffer of to-be-encoded data.
            std::vector<char> buffer;

            // The current encoding mode.
            IO::EncodingMode mode;

            // The block handlers.
            std::map<IO::EncodingMode, std::shared_ptr<Handler>> handlers;

            // Is the data being written to file now?
            bool writing;

            /**
             * Encodes the current buffer content with the handler of the current mode.
             */
            void encode()
            {
                if ((pptr() - pbase()) > 0)
                {
                    // Encode data.
                    buffer.resize(pptr() - pbase());
                    auto encoded = handlers[mode]->encode(buffer);

                    // Write to block table.
                    auto pos = blockTable.size();

                    blockTable.resize(blockTable.size() + 24);
                    auto encodedSize = Functions::Endian::write<IO::EndianType::Big, uint32_t>(encoded.size());
                    auto size = Functions::Endian::write<IO::EndianType::Big, uint32_t>(buffer.size());
                    auto checksum = Hex(Functions::Hash::md5(encoded)).data();

                    std::memcpy(blockTable.data() + pos, encodedSize.data(), encodedSize.size());
                    pos += encodedSize.size();
                    std::memcpy(blockTable.data() + pos, size.data(), size.size());
                    pos += size.size();
                    std::memcpy(blockTable.data() + pos, checksum.data(), checksum.size());

                    // Update block count.
                    if (++blockCount > UINT16_MAX)
                    {
                        throw Exceptions::IOException("Too many blocks in BLTE encoded file.");
                    }

                    // Write data.
                    this->encoded.resize(this->encoded.size() + encoded.size());
                    std::memcpy(this->encoded.data() + this->encoded.size() - encoded.size(),
                        encoded.data(), encoded.size());

                    // Empty buffer.
                    buffer.resize(0);
                    setp(buffer.data(), buffer.data());
                }
            }

        protected:
            std::streamsize xsputn(const char_type* s, std::streamsize count) override
            {
                std::streamsize written = 0;
                size_t avail = epptr() - pptr();
                size_t pos = pptr() - pbase();
                    
                if (count >= avail)
                {
                    buffer.resize(size_t(pos + count));
                    setp(buffer.data(), buffer.data() + buffer.size());
                    pbump(pos);
                }

                std::memcpy(pptr(), s, (size_t)count);
                pbump((size_t)count);
                written = count;

                return written;
            }

            int_type overflow(int_type ch = traits_type::eof()) override
            {
                if (writing)
                {
                    return std::filebuf::overflow(ch);
                }

                if (ch != traits_type::eof())
                {
                    size_t pos = pptr() - pbase();
                    buffer.resize(buffer.size() + 4096U);
                    buffer.push_back(ch);
                    setp(buffer.data(), buffer.data() + buffer.size());
                    pbump(pos + 1);
                }

                return traits_type::not_eof(ch);
            }

        public:
            /**
             * Default constructor.
             */
            WriteBuffer(const std::string basePath) :
                basePath(basePath),
                mode(IO::EncodingMode::None),
                writing(false),
                blockCount(0)
            {

            }

            /**
            * Move constructor.
            */
            WriteBuffer(WriteBuffer &&) = default;

            /**
            * Move operator.
            */
            WriteBuffer &operator= (WriteBuffer &&) = default;

            /**
            * Destructor.
            */
            virtual ~WriteBuffer() = default;

            /**
             * Registers block handlers used for encoding data blocks.
             */
            template <typename T>
            void registerHandler()
            {
                Handler* handler = new T;
                this->handlers[handler->mode()] = std::shared_ptr<Handler>(handler);
            }

            /**
             * Sets the encoding mode for the buffer.
             *
             * This encodes the current buffer content with the previous set encoding,
             * stores the encoded data in the output buffer and creates an entry
             * in the block table.
             */
            void setMode(IO::EncodingMode mode)
            {
                encode();
                this->mode = mode;
            }

            /**
             * Closes the buffer.
             */
            void close(Parsers::Binary::Reference &ref, std::string &encodingProfile)
            {
                // Enable writing-to-file flag.
                writing = true;

                // Encode last data.
                encode();

                // Parse shmem and find where to store the data.
                Parsers::Binary::ShadowMemory shmem(
                    std::string(basePath).append(PathSeparator).append("shmem"));
                ref = shmem.reserveSpace(encoded.size());

                // Open the correct file and store the data.
                std::stringstream ss;
                ss << basePath << PathSeparator
                    << "data." << std::setw(3) << std::setfill('0') << ref.file();

                std::filebuf::open("I:\\test.011", std::ios_base::out | std::ios_base::in | std::ios_base::binary);
                //std::filebuf::seekoff(ref.offset(), std::ios_base::beg, std::ios_base::out);

                auto signature = Functions::Endian::write<IO::EndianType::Little>(Signature);
                std::filebuf::xsputn(signature.data(), signature.size());
                
                if (this->blockCount > 0)
                {
                    auto headerSize = Functions::Endian::write<IO::EndianType::Big, uint32_t>(12 + blockTable.size());
                    auto flags = Functions::Endian::write<IO::EndianType::Big, uint16_t>(15);
                    auto blockCount = Functions::Endian::write<IO::EndianType::Big, uint16_t>((uint16_t)this->blockCount);

                    std::filebuf::xsputn(headerSize.data(), headerSize.size());
                    std::filebuf::xsputn(flags.data(), flags.size());
                    std::filebuf::xsputn(blockCount.data(), blockCount.size());
                    std::filebuf::xsputn(blockTable.data(), blockTable.size());
                }
                else
                {
                    for (int i = 0; i < sizeof(uint32_t); ++i)
                    {
                        std::filebuf::sputc('\0');
                    }
                }
                std::filebuf::xsputn(encoded.data(), encoded.size());
                std::filebuf::close();
            }
        };
    }
}