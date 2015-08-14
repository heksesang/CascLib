#pragma once

#include <string>
#include <memory>
#include <map>
#include <stdint.h>
#include <array>
#include <fstream>

#include "Common.hpp"

#include "Shared/Utils.hpp"
#include "CascContainer.hpp"

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
        virtual std::string findHash(std::string filename) const = 0;

    protected:
        // The container.
        std::shared_ptr<CascContainer> container;

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
        void checkForErrors(std::unique_ptr<std::istream>&& stream) const
        {
            if (stream->fail())
            {
                throw std::exception("Stream is in an invalid state.");
            }
        }

    public:
        /**
         * Default constructor.
         */
        CascRootHandler(std::shared_ptr<CascContainer> container)
            : container(container)
        {
            parse();
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
         */
        virtual void parse() = 0;
    };
}