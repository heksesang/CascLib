#pragma once

#include <fstream>
#include <string>
#include "CascStream.hpp"
#include "StreamOps.hpp"
#include "Shared/Hex.hpp"
//#include "Shared/Utils.hpp"

namespace Casc
{
    using namespace Casc::Shared;
    using namespace Casc::Shared::Endian;

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
            Hex<16> hex(hash);

            for (unsigned int i = 0; i < chunkHeadsA.size(); ++i)
            {
                Hex<16> current(chunkHeadsA.at(i).first);
                Hex<16> next((i + 1) >= chunkHeadsA.size() ?
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
                        
                        if (hex.data() == chunk->hash)
                        {
                            std::array<uint8_t, 9> temp;
                            std::memcpy(&temp[0], &chunk->key[0], temp.size());

                            return Hex<9>(temp).string();
                        }

                        ++chunk;
                    }
                }
            }

            for (unsigned int i = 0; i < chunkHeadsB.size(); ++i)
            {
                Hex<16> current(chunkHeadsB.at(i).first);
                Hex<16> next((i + 1) >= chunkHeadsB.size() ?
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

                        if (hex.data() == chunk->hash)
                        {
                            std::array<uint8_t, 9> temp;
                            std::memcpy(&temp[0], &chunk->key[0], temp.size());

                            return Hex<9>(temp).string();
                        }

                        ++chunk;
                    }
                }
            }

            throw std::exception("Key not found");
        }

    private:
        // The header size of an encoding file.
        const unsigned int HeaderSize = 22U;

        // The size of each chunk body (second block for each table).
        const unsigned int ChunkBodySize = 4096U;

        // The encoding file stream.
        std::unique_ptr<std::istream> stream;

        /**
         * Reads data from a stream and puts it in a struct.
         *
         * @param T     the type of the struct.
         * @param input the input stream.
         * @param value the output object to write the data to.
         * @param big   true if big endian.
         * @return      the data.
         */
        template <typename T>
        const T &read(T &value, bool big = false) const
        {
            char b[sizeof(T)];
            stream->read(b, sizeof(T));

            return value = big ? readBE<T>(b) : readLE<T>(b);
        }

        /**
         * Throws if the fail or bad bit are set on the stream.
         */
        void checkForErrors()
        {
            if (stream->fail())
            {
                throw std::exception("Stream is in an invalid state.");
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
        CascEncoding(std::unique_ptr<std::istream> stream)
            : CascEncoding()
        {
            parse(std::move(stream));
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
        CascEncoding &CascEncoding::operator= (CascEncoding &&) = default;

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
        void parse(std::unique_ptr<std::istream> stream)
        {
            this->stream = std::move(stream);

            char magic[2];
            this->stream->read(magic, 2);

            if (magic[0] != 0x45 || magic[1] != 0x4E)
            {
                throw std::exception((std::string("Invalid file magic")).c_str());
            }

            this->stream->seekg(7, std::ios_base::cur);

            uint32_t tableSizeA;
            uint32_t tableSizeB;

            read(tableSizeA, true);
            read(tableSizeB, true);

            uint32_t stringTableSize;

            this->stream->seekg(1, std::ios_base::cur);

            read(stringTableSize, true);

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