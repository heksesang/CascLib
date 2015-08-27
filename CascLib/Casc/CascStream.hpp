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

#include <type_traits>
#include <iostream>
#include <memory>

#include "Common.hpp"

namespace Casc
{
    /**
     * A stream that uses the CascBuffer as the underlying buffer.
     * The class provides methods to easily control the underlying buffer.
     */
    template <bool Writeable>
    class CascStream : public std::conditional<Writeable, std::ostream, std::istream>::type
    {
        typedef std::filebuf write_buf_type;
        typedef CascBuffer read_buf_type;
#ifdef _MSC_VER
        typedef std::common_type<write_buf_type, read_buf_type>::type base_buf_type;
#else
        typedef std::filebuf base_buf_type;
#endif

        typedef typename std::conditional<Writeable,
            std::ostream, std::istream>::type base_type;
        typedef typename std::conditional<Writeable,
            write_buf_type, read_buf_type>::type buf_type;

        // The underlying buffer.
        base_buf_type* buffer;

        // Chunk handlers
        std::map<char, std::shared_ptr<CascBlteHandler>> handlers;

    public:
        /**
         * Default constructor.
         */
        CascStream()
            : buffer(reinterpret_cast<buf_type*>(this->rdbuf())),
              base_type(new buf_type())
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
        virtual ~CascStream() noexcept override
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
            if (Writeable)
            {
                auto buf = reinterpret_cast<write_buf_type*>(buffer);
                if (buf->pubseekoff(offset, std::ios_base::beg, std::ios_base::out) != (std::streampos)offset)
                {
                    throw Casc::Exceptions::GenericException("Failed to open CASC archive for writing at given position.");
                }
            }
            else
            {
                auto buf = reinterpret_cast<read_buf_type*>(buffer);
                buf->open(offset);
            }
        }

        /**
         * Opens a file in a CASC file.
         *
         * @param filename	the filename of the CASC file.
         * @param offset	the offset where the file starts.
         */
        void open(const char *filename, size_t offset)
        {
            if (Writeable)
            {
                auto buf = reinterpret_cast<write_buf_type*>(buffer);
                buf->open(filename, std::ios_base::out | std::ios_base::in | std::ios_base::binary);
                if (buf->pubseekoff(offset, std::ios_base::beg, std::ios_base::out) != (std::streampos)offset)
                {
                    throw Casc::Exceptions::GenericException("Failed to open CASC archive for writing at given position.");
                }
            }
            else
            {
                auto buf = reinterpret_cast<read_buf_type*>(buffer);
                buf->open(filename, offset);
            }
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
            if (!Writeable)
            {
                for (std::shared_ptr<CascBlteHandler> handler : handlers)
                    this->handlers[handler->compressionMode()] = handler;

                reinterpret_cast<read_buf_type*>(this->rdbuf())->registerHandlers(handlers);
            }
        }

        template <typename T>
        void registerHandler()
        {
            if (!Writeable)
            {
                CascBlteHandler* handler = new T;
                this->handlers[handler->compressionMode()] = std::shared_ptr<CascBlteHandler>(handler);

                reinterpret_cast<read_buf_type*>(this->rdbuf())->registerHandlers({ this->handlers[handler->compressionMode()] });
            }
        }
    };
}