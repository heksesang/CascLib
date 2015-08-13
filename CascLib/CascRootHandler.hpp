#pragma once

#include <string>
#include <memory>
#include <map>
#include <stdint.h>
#include <array>
#include <fstream>
#include "Shared/Utils.hpp"
#include "CascTraits.hpp"

namespace Casc
{
    using namespace Casc::Shared;

    /**
     * Maps filename to file content MD5 hash.
     * 
     * TODO: Finish implementation, need different handling per game.
     */
    class CascRootHandler
    {
    public:
        /**
         * Find the file content hash for the given filename.
         *
         * @param filename  the filename.
         * @return          the hash in hex format.
         */
        virtual std::string findHash(std::string filename) = 0;

    protected:
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

//#pragma pack(push, 1)
//        struct ChunkHead
//        {
//            std::array<uint8_t, 4> unk;
//        };
//
//        struct ChunkBody
//        {
//            std::array<uint8_t, 16> hash;
//            std::array<uint8_t, 4> lookupB;
//            std::array<uint8_t, 4> lookupA;
//        };
//#pragma pack(pop)
//
//        // Hash table with the file hashes mapped with lookup as key.
//        std::map<std::pair<uint32_t, uint32_t>, std::string> hashes;

    public:
        /**
         * Default constructor.
         */
        CascRootHandler()
        {

        }

        /**
         * Constructor.
         *
         * @param stream    pointer to the stream.
         */
        CascRootHandler(std::shared_ptr<std::istream> stream)
            : CascRootHandler()
        {
            parse(stream);
        }

        /**
         * Constructor.
         *
         * @param path      path to the root file.
         */
        CascRootHandler(std::string path)
            : CascRootHandler()
        {
            parse(path);
        }

        /**
         * Move constructor.
         */
        CascRootHandler(CascRootHandler &&) = default;

        /**
        * Move operator.
        */
        CascRootHandler &CascRootHandler::operator= (CascRootHandler &&) = default;

        /**
         * Destructor.
         */
        virtual ~CascRootHandler()
        {
        }

        /**
         * The file magic of the root file.
         */
        virtual std::array<char, 4> fileMagic() const = 0;

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
        virtual void parse(std::shared_ptr<std::istream> stream) = 0;
    };
}