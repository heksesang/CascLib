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

#include "../../Common.hpp"
#include "../../Exceptions.hpp"

#include "Index.hpp"

namespace Casc
{
    namespace Parsers
    {
        namespace Binary
        {
            using namespace Casc::Functions;
            using namespace Casc::Functions::Endian;
            using namespace Casc::Functions::Hash;

            /**
             * Maps file content MD5 hash to file key.
             */
            class Encoding
            {
            public:
                /**
                 * Find the file key for the given hash.
                 */
                std::vector<Hex> find(const std::string hash) const
                {
                    Hex target(hash);
                    std::vector<Hex> keys;

                    if ((keys = searchTable(target, chunkHeadsA, chunksOffsetA, hashSizeA)).empty())
                    {
                        throw Exceptions::HashDoesNotExistException(hash);
                    }

                    return keys;
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
                 */
                template <IO::EndianType Endian = IO::EndianType::Little, typename T>
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
                        throw Exceptions::IOException("Stream is in an invalid state.");
                    }
                }

#pragma pack(push, 1)
                struct ChunkHead
                {
                    std::array<uint8_t, 16> first;
                    std::array<uint8_t, 16> hash;
                };

                class ChunkBody
                {
                public:
                    ChunkBody(size_t hashSize)
                        : hash(hashSize), keys(1, std::vector<char>(hashSize))
                    {

                    }

                    uint32_t fileSize;
                    std::vector<char> hash;
                    std::vector<std::vector<char>> keys;
                };
#pragma pack(pop)

                // The size of the hashes in table A.
                size_t hashSizeA;

                // The size of the hashes in table B.
                size_t hashSizeB;

                // The headers for the chunks in table A.
                std::vector<ChunkHead> chunkHeadsA;

                // The headers for the chunks in table B.
                std::vector<ChunkHead> chunkHeadsB;

                // The offset of the chunks in table A.
                std::streamsize chunksOffsetA;

                // The offset of the chunks in table B.
                std::streamsize chunksOffsetB;

                // The encoding profiles
                std::vector<std::vector<Text::EncodingBlock>> profiles;

                /**
                 * Search the table for a given hash.
                 */
                std::vector<Hex> searchTable(Hex target, const std::vector<ChunkHead> &heads, std::streamsize offset, size_t hashSize) const
                {
                    std::vector<Hex> keys;

                    for (unsigned int i = 0; keys.empty() && i < heads.size(); ++i)
                    {
                        Hex first(heads.at(i).first);

                        if (target > first)
                        {
                            stream->seekg(offset + ChunkBodySize * (heads.size() - 1 - i), std::ios_base::beg);

                            std::array<char, 4096> data;
                            stream->read(data.data(), 4096);

                            Hex expected(heads.at(i).hash);
                            Hex actual(md5(data));

                            if (actual != expected)
                            {
                                throw Exceptions::InvalidHashException(lookup3(expected, 0), lookup3(actual, 0), "");
                            }

                            for (auto it = data.begin(), end = data.end(); keys.empty() && it < end;)
                            {
                                auto keyCount =
                                    Endian::read<IO::EndianType::Little, uint16_t>(it);
                                it += sizeof(keyCount);

                                if (keyCount == 0)
                                    break;

                                auto fileSize =
                                    Endian::read<IO::EndianType::Big, uint32_t>(it);
                                it += sizeof(fileSize);

                                std::vector<char> hash(hashSizeA);
                                std::copy(it, it + hashSizeA, hash.begin());
                                it += hashSizeA;

                                Hex current(hash);

                                for (auto i = 0U; i < keyCount; ++i)
                                {
                                    std::vector<char> key(hashSizeA);
                                    std::copy(it, it + hashSizeA, key.begin());
                                    it += hashSizeA;

                                    if (target == current)
                                    {
                                        keys.emplace_back(key);
                                    }
                                }
                            }
                        }
                    }

                    return keys;
                }

                /**
                * Parse an encoding file.
                */
                void parse(std::shared_ptr<std::istream> stream)
                {
                    this->stream = stream;

                    uint16_t signature;
                    if (read(signature) != 0x4E45)
                    {
                        throw Exceptions::InvalidSignatureException(signature, 0x4E45);
                    }

                    this->stream->seekg(1, std::ios_base::cur);

                    uint8_t hashSizeA;
                    uint8_t hashSizeB;

                    this->hashSizeA = read<IO::EndianType::Little, uint8_t>(hashSizeA);
                    this->hashSizeB = read<IO::EndianType::Little, uint8_t>(hashSizeB);

                    this->stream->seekg(4, std::ios_base::cur);

                    uint32_t tableSizeA;
                    uint32_t tableSizeB;

                    read<IO::EndianType::Big>(tableSizeA);
                    read<IO::EndianType::Big>(tableSizeB);

                    uint32_t stringTableSize;

                    this->stream->seekg(1, std::ios_base::cur);

                    read<IO::EndianType::Big>(stringTableSize);

                    while (this->stream->tellg() < (HeaderSize + stringTableSize - 1))
                    {
                        std::string profile;
                        std::getline(*this->stream, profile, '\0');

                        profiles.emplace_back(Text::EncodingBlock::parse(profile));
                    }

                    for (unsigned int i = 0; i < tableSizeA; ++i)
                    {
                        ChunkHead head;
                        this->stream->read(reinterpret_cast<char*>(&head), sizeof(ChunkHead));

                        chunkHeadsA.push_back(head);
                    }

                    std::reverse(chunkHeadsA.begin(), chunkHeadsA.end());

                    chunksOffsetA = HeaderSize + stringTableSize + tableSizeA * sizeof(ChunkHead);

                    this->stream->seekg(tableSizeA * ChunkBodySize, std::ios_base::cur);

                    for (unsigned int i = 0; i < tableSizeB; ++i)
                    {
                        ChunkHead head;
                        this->stream->read(reinterpret_cast<char*>(&head), sizeof(ChunkHead));

                        chunkHeadsB.push_back(head);
                    }

                    std::reverse(chunkHeadsB.begin(), chunkHeadsB.end());

                    chunksOffsetB = HeaderSize + stringTableSize +
                        tableSizeA * sizeof(ChunkHead) + tableSizeA * ChunkBodySize +
                        tableSizeB * sizeof(ChunkHead);

                    checkForErrors();
                }

            public:
                /**
                 * Constructor.
                 */
                Encoding(Hex hash, std::shared_ptr<Parsers::Binary::Index> index,
                         std::shared_ptr<IO::StreamAllocator> allocator)
                    : chunksOffsetA(0), chunksOffsetB(0)
                {
                    auto ref = index->find(hash.begin(), hash.begin() + 9);
                    auto stream = allocator->allocate<false>(ref);

                    parse(stream);
                }

                /**
                * Copy constructor.
                */
                Encoding(const Encoding &) = default;

                /**
                 * Move constructor.
                 */
                Encoding(Encoding &&) = default;

                /**
                * Copy operator.
                */
                Encoding &operator= (const Encoding &) = default;

                /**
                 * Move operator.
                 */
                Encoding &operator= (Encoding &&) = default;

                /**
                 * Destructor.
                 */
                virtual ~Encoding() = default;

                /**
                * Inserts a file record.
                */
                std::vector<char> insert(const std::string hash, const std::string key, size_t fileSize) const
                {
                    std::vector<char> v(22 + chunkHeadsA.size() * (hashSizeA * 2 + 4096) +
                        chunkHeadsB.size() * (hashSizeB * 2 + 4096));

                    std::map<Hex, ChunkBody> bodies;

                    size_t count = 0U;
                    for (auto i = 0U; i < chunkHeadsA.size(); ++i)
                    {
                        stream->seekg(chunksOffsetA + ChunkBodySize * i, std::ios_base::beg);

                        std::array<char, 4096> data;
                        stream->read(data.data(), 4096);

                        for (auto it = data.begin(), end = data.end(); it < end;)
                        {
                            ChunkBody body(hashSizeA);

                            auto keyCount =
                                Endian::read<IO::EndianType::Little, uint16_t>(it);
                            it += sizeof(keyCount);

                            body.keys.resize(keyCount, std::vector<char>(hashSizeA));

                            if (keyCount == 0)
                                break;

                            body.fileSize =
                                Endian::read<IO::EndianType::Big, uint32_t>(it);
                            it += sizeof(fileSize);

                            std::copy(it, it + hashSizeA, body.hash.begin());
                            it += hashSizeA;

                            for (auto i = 0U; i < keyCount; ++i)
                            {
                                std::copy(it, it + hashSizeA, body.keys[i].begin());
                                it += hashSizeA;
                            }

                            bodies.insert({ Hex(body.hash), body });
                        }
                    }

                    /*for (auto i = 0U; i < chunkHeadsB.size(); ++i)
                    {
                        stream->seekg(chunksOffsetB + ChunkBodySize * i, std::ios_base::beg);

                        std::array<char, 4096> data;
                        stream->read(data.data(), 4096);

                        for (auto it = data.begin(), end = data.end(); it < end;)
                        {
                            ChunkBody body(hashSizeB);

                            auto keyCount =
                                Endian::read<IO::EndianType::Little, uint16_t>(it);
                            it += sizeof(keyCount);

                            body.keys.resize(keyCount, std::vector<char>(hashSizeB));

                            if (keyCount == 0)
                                break;

                            body.fileSize =
                                Endian::read<IO::EndianType::Little, uint32_t>(it);
                            it += sizeof(fileSize);

                            std::copy(it, it + hashSizeB, body.hash.begin());
                            it += hashSizeB;

                            for (auto i = 0U; i < keyCount; ++i)
                            {
                                std::copy(it, it + hashSizeB, body.keys[i].begin());
                                it += hashSizeB;
                            }

                            bodies.insert({ Hex(body.hash), body });
                        }
                    }*/

                    auto magic = write<IO::EndianType::Little, uint16_t>(0x4E45);
                    auto unk1 = write<IO::EndianType::Big, uint8_t>(1);
                    auto checksumSizeA = write<IO::EndianType::Big, uint8_t>(16);
                    auto checksumSizeB = write<IO::EndianType::Big, uint8_t>(16);
                    auto flagsA = write<IO::EndianType::Big, uint16_t>(4);
                    auto flagsB = write<IO::EndianType::Big, uint16_t>(4);
                    auto countA = write<IO::EndianType::Big, uint32_t>(chunkHeadsA.size());
                    auto countB = write<IO::EndianType::Big, uint32_t>(chunkHeadsB.size());
                    auto unk2 = write<IO::EndianType::Big, uint8_t>(0);
                    auto stringBlockSize = write<IO::EndianType::Big, uint32_t>(0);

                    return v;
                }
            };
        }
    }
}