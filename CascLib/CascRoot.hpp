#pragma once

#include <string>
#include <memory>
#include <map>
#include <stdint.h>
#include <array>
#include <fstream>
#include "Shared/Utils.hpp"

namespace Casc
{
    using namespace Casc::Shared;

    /**
     * Maps filename to file content MD5 hash.
     * 
     * TODO: Finish implementation, need different handling per game.
     */
    class CascRoot
    {
    public:
        /**
         * Find the file content hash for the given filename.
         *
         * @param filename  the filename.
         * @return          the hash in hex format.
         */
        std::string findHash(std::string filename)
        {
            std::pair<uint32_t, uint32_t> lookup;
            lookup = Hash::lookup3(filename, lookup);

            return hashes[lookup];
        }

    private:
        // The header size of a root file.
        const unsigned int HeaderSize = 12U;

        // The root file stream.
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
            std::array<uint8_t, 4> unk;
        };

        struct ChunkBody
        {
            std::array<uint8_t, 16> hash;
            std::array<uint8_t, 4> lookupB;
            std::array<uint8_t, 4> lookupA;
        };
#pragma pack(pop)

        // Hash table with the file hashes mapped with lookup as key.
        std::map<std::pair<uint32_t, uint32_t>, std::string> hashes;

    public:
        /**
         * Default constructor.
         */
        CascRoot()
        {

        }

        /**
         * Constructor.
         *
         * @param stream    pointer to the stream.
         */
        CascRoot(std::unique_ptr<std::istream> stream)
            : CascRoot()
        {
            parse(std::move(stream));
        }

        /**
         * Constructor.
         *
         * @param path      path to the root file.
         */
        CascRoot(std::string path)
            : CascRoot()
        {
            parse(path);
        }

        /**
         * Move constructor.
         */
        CascRoot(CascRoot &&) = default;

        /**
        * Move operator.
        */
        CascRoot &CascRoot::operator= (CascRoot &&) = default;

        /**
         * Destructor.
         */
        virtual ~CascRoot()
        {
        }

        /**
         * Parse a root file.
         *
         * @param path      path to the root file.
         */
        void parse(std::string path)
        {
            std::unique_ptr<std::istream> fs =
                std::make_unique<std::ifstream>(path, std::ios_base::in | std::ios_base::binary);
            parse(std::move(fs));
        }

        /**
         * Parse a root file.
         *
         * @param stream    pointer to the stream.
         */
        void parse(std::unique_ptr<std::istream> stream)
        {
            this->stream = std::move(stream);

            uint32_t tableSize;

            read(tableSize, true);

            this->stream->seekg(8, std::ios_base::cur);

            for (unsigned int i = 0; i < tableSize; ++i)
            {
                // Read hashes
            }

            checkForErrors();
        }
    };
}