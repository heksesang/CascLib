#pragma once

#include <iostream>
#include <memory>
#include "CascBuffer.hpp"

namespace Casc
{
    /**
     * A stream that uses the CascBuffer as the underlying buffer.
     * The class provides methods to easily control the underlying buffer.
     */
    class CascStream : public std::istream
    {
        // The underlying buffer which allows direct data streaming.
        std::unique_ptr<CascBuffer> buffer;

    public:
        /**
         * Default constructor.
         */
        CascStream()
            : buffer(reinterpret_cast<CascBuffer*>(rdbuf())), std::istream(new CascBuffer())
        {
            buffer->registerHandler<ZlibHandler<>>();
        }

        /**
         * Constructor.
         *
         * @param filename	the filename of the CASC file.
         * @param offset	the offset where the file starts.
         */
        CascStream(const std::string &filename, size_t offset)
            : CascStream()
        {
            open(filename, offset);
        }

        /**
        * Move constructor.
        */
        CascStream(CascStream &&) = default;

        /**
         * Destructor.
         */
        ~CascStream() override
        {
            if (is_open())
            {
                close();
            }
        }

        /**
         * Move operator.
         */
        CascStream &CascStream::operator= (CascStream &&) = default;

        /**
         * Opens a file from the currently opened CASC file.
         *
         * @param offset	the offset where the file starts.
         */
        void open(size_t offset)
        {
            buffer->open(offset);
        }

        /**
         * Opens a file in a CASC file.
         *
         * @param filename	the filename of the CASC file.
         * @param offset	the offset where the file starts.
         */
        void open(const char *filename, size_t offset)
        {
            buffer->open(filename, offset);
        }

        /**
         * Opens a file in a CASC file.
         *
         * @param filename	the filename of the CASC file.
         * @param offset	the offset where the file starts.
         */
        void open(const std::string &filename, size_t offset)
        {
            open(filename.c_str(), offset);
        }

        /**
         * Closes the currently opened CASC file.
         */
        void close()
        {
            buffer->close();
        }

        /**
         * Checks if the buffer is open.
         *
         * @return true if the buffer is open, false if the buffer is closed.
         */
        bool is_open() const
        {
            return buffer->is_open();
        }
    };
}