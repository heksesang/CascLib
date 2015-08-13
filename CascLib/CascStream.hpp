#pragma once

#include <iostream>
#include <memory>
#include "CascBlteHandler.hpp"
#include "CascBuffer.hpp"

namespace Casc
{
    /**
     * A stream that uses the CascBuffer as the underlying buffer.
     * The class provides methods to easily control the underlying buffer.
     */
    class BaseCascStream : public std::istream
    {
        // The underlying buffer which allows direct data streaming.
        std::unique_ptr<CascBuffer> buffer;

        // Chunk handlers
        std::map<char, std::shared_ptr<CascBlteHandler>> handlers;

    public:
        /**
         * Default constructor.
         */
        BaseCascStream()
            : buffer(reinterpret_cast<CascBuffer*>(rdbuf())), std::istream(new CascBuffer())
        {
            registerHandler<DefaultHandler<>>();
        }

        BaseCascStream(std::vector<std::shared_ptr<CascBlteHandler>> handlers)
            : BaseCascStream()
        {
            registerHandlers(handlers);
        }

        /**
         * Constructor.
         *
         * @param filename	the filename of the CASC file.
         * @param offset	the offset where the file starts.
         * @param handlers  the chunk handlers.
         */
        BaseCascStream(const std::string &filename, size_t offset,
            std::vector<std::shared_ptr<CascBlteHandler>> handlers = {})
            : BaseCascStream(handlers)
        {
            open(filename, offset);
        }

        /**
        * Move constructor.
        */
        BaseCascStream(BaseCascStream &&) = default;

        /**
         * Destructor.
         */
        virtual ~BaseCascStream() override
        {
            if (is_open())
            {
                close();
            }
        }

        /**
         * Move operator.
         */
        BaseCascStream &BaseCascStream::operator= (BaseCascStream &&) = default;

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

        void registerHandlers(std::vector<std::shared_ptr<CascBlteHandler>> handlers)
        {
            for (std::shared_ptr<CascBlteHandler> handler : handlers)
                this->handlers[handler->compressionMode()] = handler;

            reinterpret_cast<CascBuffer*>(this->rdbuf())->registerHandlers(handlers);
        }

        template <typename T>
        void registerHandler()
        {
            CascBlteHandler* handler = new T;
            this->handlers[handler->compressionMode()] = std::shared_ptr<CascBlteHandler>(handler);
            reinterpret_cast<CascBuffer*>(this->rdbuf())->registerHandlers({ this->handlers[handler->compressionMode()] });
        }
    };

    typedef BaseCascStream CascStream;
}