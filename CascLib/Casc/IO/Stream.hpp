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

#include "../Common.hpp"

#include "Buffer.hpp"

namespace Casc
{
    namespace IO
    {
        /**
         * A stream that uses the IO::Buffer as the underlying buffer.
         * The class provides methods to easily control the underlying buffer.
         */
        template <bool Writeable>
        class Stream : public std::conditional<Writeable, std::ostream, std::istream>::type
        {
            typedef std::filebuf write_buf_type;
            typedef IO::Buffer read_buf_type;
#ifdef _MSC_VER_
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
            std::map<char, std::shared_ptr<Handler>> handlers;

        public:
            /**
             * Default constructor.
             */
            Stream()
                : buffer(reinterpret_cast<buf_type*>(this->rdbuf())),
                base_type(new buf_type())
            {
                registerHandler<Impl::DefaultHandler>();
            }

            Stream(std::vector<std::shared_ptr<Handler>> handlers)
                : Stream()
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
            Stream(const std::string filename, size_t offset,
                std::vector<std::shared_ptr<Handler>> handlers = {})
                : Stream(handlers)
            {
                open(filename, offset);
            }

            /**
            * Move constructor.
            */
            Stream(Stream &&) = default;

            /**
             * Destructor.
             */
            virtual ~Stream() noexcept override
            {
                if (is_open())
                {
                    close();
                }
            }

            /**
             * Move operator.
             */
            Stream &operator= (Stream &&) = default;

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
                        throw Exceptions::IOException("Failed to open CASC archive for writing at given position.");
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
                        throw Exceptions::IOException("Failed to open CASC archive for writing at given position.");
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
            void open(const std::string filename, size_t offset)
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

            void registerHandlers(std::vector<std::shared_ptr<Handler>> handlers)
            {
                if (!Writeable)
                {
                    for (std::shared_ptr<Handler> handler : handlers)
                        this->handlers[handler->mode()] = handler;

                    reinterpret_cast<read_buf_type*>(this->rdbuf())->registerHandlers(handlers);
                }
            }

            template <typename T>
            void registerHandler()
            {
                if (!Writeable)
                {
                    Handler* handler = new T;
                    this->handlers[handler->mode()] = std::shared_ptr<Handler>(handler);

                    reinterpret_cast<read_buf_type*>(this->rdbuf())->registerHandlers({ this->handlers[handler->mode()] });
                }
            }
        };
    }
}