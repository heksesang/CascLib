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
#include <string>

#include "Common.hpp"

namespace Casc
{
    using namespace Casc::Shared;
    using namespace Casc::Shared::Functions;
    using namespace Casc::Shared::Functions::Endian;

    /**
     * Maps file content MD5 hash to file key.
     */
    class CascEncoding
    {
    public:
        /**
         * Find the file key for the given hash.
         *
         * @param hash  the MD5 hash of the file content.
         * @return      the key in hex format.
         */
        std::string findKey(const std::string &hash) const
        {
            Hex hex(hash);

            for (unsigned int i = 0; i < chunkHeadsA.size(); ++i)
            {
                Hex current(chunkHeadsA.at(i).first);
                Hex next((i + 1) >= chunkHeadsA.size() ?
                    std::array<uint8_t, 16> { (uint8_t)0xFF } :
                    chunkHeadsA.at(i + 1).first);

                if ((i + 1) >= chunkHeadsA.size() ||
                    (hash >= current.string() && hash < next.string()))
                {
                    stream->seekg(chunksOffsetA + ChunkBodySize * i, std::ios_base::beg);

                    char data[4096];
                    stream->read(data, 4096);

                    ChunkBody* chunk = reinterpret_cast<ChunkBody*>(data);

                    while (true)
                    {
                        if (chunk->unk != 1)
                            break;
                        
                        if (std::equal(
                                hex.data().begin(), hex.data().end(),
                                chunk->hash.begin(), chunk->hash.end()))
                        {
                            std::array<uint8_t, 9> temp;
                            std::memcpy(&temp[0], &chunk->key[0], temp.size());

                            return Hex(temp).string();
                        }

                        ++chunk;
                    }
                }
            }

            for (unsigned int i = 0; i < chunkHeadsB.size(); ++i)
            {
                Hex current(chunkHeadsB.at(i).first);
                Hex next((i + 1) >= chunkHeadsB.size() ?
                    std::array<uint8_t, 16> { (uint8_t)0xFF } :
                    chunkHeadsB.at(i + 1).first);

                if ((i + 1) >= chunkHeadsB.size() ||
                    (hash >= current.string() && hash < next.string()))
                {
                    stream->seekg(chunksOffsetB + ChunkBodySize * i, std::ios_base::beg);

                    char data[4096];
                    stream->read(data, 4096);

                    ChunkBody* chunk = reinterpret_cast<ChunkBody*>(data);

                    while (true)
                    {
                        if (chunk->unk != 1)
                            break;

                        if (std::equal(
                                hex.data().begin(), hex.data().end(),
                                chunk->hash.begin(), chunk->hash.end()))
                        {
                            std::array<uint8_t, 9> temp;
                            std::memcpy(&temp[0], &chunk->key[0], temp.size());

                            return Hex(temp).string();
                        }

                        ++chunk;
                    }
                }
            }

            throw FileNotFoundException(hash);
        }

    private:
        // The header size of an encoding file.
        static const unsigned int HeaderSize = 22U;

        // The size of each chunk body (second block for each table).
        static const unsigned int ChunkBodySize = 4096U;

        // The encoding file stream.
        std::shared_ptr<std::istream> stream;

        /**
         * Reads data from a stream and puts it in a struct.
         *
         * @param T     the type of the struct.
         * @param input the input stream.
         * @param value the output object to write the data to.
         * @param big   true if big endian.
         * @return      the data.
         */
        template <EndianType Endian = EndianType::Little, typename T>
        const T &read(T &value) const
        {
            char b[sizeof(T)];
            stream->read(b, sizeof(T));

            return value = Endian::read<Endian, T>(b);
        }

        /**
         * Throws if the fail or bad bit are set on the stream.
         */
        void checkForErrors() const
        {
            if (stream->fail())
            {
                throw GenericException("Stream is in an invalid state.");
            }
        }

#pragma pack(push, 1)
        struct ChunkHead
        {
            std::array<uint8_t, 16> first;
            std::array<uint8_t, 16> hash;
        };

        struct ChunkBody
        {
            uint16_t unk;
            uint32_t fileSize;
            std::array<uint8_t, 16> hash;
            std::array<uint8_t, 16> key;
        };
#pragma pack(pop)

        // The headers for the chunks in table A.
        std::vector<ChunkHead> chunkHeadsA;

        // The headers for the chunks in table B.
        std::vector<ChunkHead> chunkHeadsB;

        // The offset of the chunks in table A.
        std::streamsize chunksOffsetA;

        // The offset of the chunks in table B.
        std::streamsize chunksOffsetB;

    public:
        /**
         * Default constructor.
         */
        CascEncoding()
            : chunksOffsetA(0), chunksOffsetB(0)
        {

        }

        /**
         * Constructor.
         *
         * @param stream    pointer to the stream.
         */
        CascEncoding(std::shared_ptr<std::istream> stream)
            : CascEncoding()
        {
            parse(stream);
        }

        /**
         * Constructor.
         *
         * @param path      path to the encoding file.
         */
        CascEncoding(std::string path)
            : CascEncoding()
        {
            parse(path);
        }

        /**
         * Move constructor.
         */
        CascEncoding(CascEncoding &&) = default;

        /**
         * Move operator.
         */
        CascEncoding &operator= (CascEncoding &&) = default;

        /**
         * Destructor.
         */
        virtual ~CascEncoding()
        {
        }

        /**
         * Parse an encoding file.
         *
         * @param path      path to the encoding file.
         */
        void parse(std::string path)
        {
            std::unique_ptr<std::istream> fs =
                std::make_unique<std::ifstream>(path, std::ios_base::in | std::ios_base::binary);
            parse(std::move(fs));
        }

        /**
         * Parse an encoding file.
         *
         * @param stream    pointer to the stream.
         */
        void parse(std::shared_ptr<std::istream> stream)
        {
            this->stream = stream;

            char magic[2];
            this->stream->read(magic, 2);

            if (magic[0] != 0x45 || magic[1] != 0x4E)
            {
                throw InvalidSignatureException(*reinterpret_cast<uint16_t*>(&magic), 0x454E);
            }

            this->stream->seekg(7, std::ios_base::cur);

            uint32_t tableSizeA;
            uint32_t tableSizeB;

            read<EndianType::Big>(tableSizeA);
            read<EndianType::Big>(tableSizeB);

            uint32_t stringTableSize;

            this->stream->seekg(1, std::ios_base::cur);

            read<EndianType::Big>(stringTableSize);

            this->stream->seekg(stringTableSize, std::ios_base::cur);

            for (unsigned int i = 0; i < tableSizeA; ++i)
            {
                ChunkHead head;
                this->stream->read(reinterpret_cast<char*>(&head), sizeof(ChunkHead));

                chunkHeadsA.push_back(head);
            }

            chunksOffsetA = HeaderSize + stringTableSize + tableSizeA * sizeof(ChunkHead);

            this->stream->seekg(tableSizeA * ChunkBodySize, std::ios_base::cur);

            for (unsigned int i = 0; i < tableSizeB; ++i)
            {
                ChunkHead head;
                this->stream->read(reinterpret_cast<char*>(&head), sizeof(ChunkHead));

                chunkHeadsB.push_back(head);
            }

            chunksOffsetB = HeaderSize + stringTableSize + tableSizeA * sizeof(ChunkHead) + tableSizeA * ChunkBodySize;

            checkForErrors();
        }
    };
}