#pragma once

#include <type_traits>
#include <iostream>
#include <memory>

#include "Common.hpp"

#include "CascBlteHandler.hpp"
#include "CascBuffer.hpp"

namespace Casc
{
    /**
     * A stream that uses the CascBuffer as the underlying buffer.
     * The class provides methods to easily control the underlying buffer.
     */
    template <bool Writeable>
    class CascStream : public std::conditional<Writeable, std::iostream, std::istream>::type
    {
        // The underlying buffer which allows direct data streaming.
        std::unique_ptr<CascBuffer> buffer;

        // Chunk handlers
        std::map<char, std::shared_ptr<CascBlteHandler>> handlers;

    public:
        /**
         * Default constructor.
         */
        CascStream()
            : buffer(reinterpret_cast<CascBuffer*>(this->rdbuf())), std::istream(new CascBuffer())
        {
            registerHandler<DefaultHandler>();
        }

        CascStream(std::vector<std::shared_ptr<CascBlteHandler>> handlers)
            : CascStream()
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
        CascStream(const std::string &filename, size_t offset,
            std::vector<std::shared_ptr<CascBlteHandler>> handlers = {})
            : CascStream(handlers)
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
        virtual ~CascStream() override
        {
            if (is_open())
            {
                close();
            }
        }

        /**
         * Move operator.
         */
        CascStream &operator= (CascStream &&) = default;

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
}