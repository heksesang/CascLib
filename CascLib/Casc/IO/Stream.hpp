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

#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>

#include "../Common.hpp"

#include "../Parsers/Binary/Reference.hpp"
#include "ReadBuffer.hpp"
#include "WriteBuffer.hpp"

namespace Casc
{
    namespace IO
    {
        /**
         * A stream that uses the Buffer as the underlying buffer.
         */
        template <bool Writeable>
        class Stream : public std::conditional<Writeable, std::ostream, std::istream>::type
        {
        public:
            typedef std::function<void(Parsers::Binary::Reference, Hex, size_t)> insert_func;

        private:
            // Typedefs
            typedef WriteBuffer write_buf_type;
            typedef ReadBuffer read_buf_type;
            typedef std::filebuf base_buf_type;

            typedef typename std::conditional<Writeable,
                std::ostream, std::istream>::type base_type;
            typedef typename std::conditional<Writeable,
                write_buf_type, read_buf_type>::type buf_type;

            // The underlying buffer.
            std::unique_ptr<base_buf_type> buffer;

            // The base path of the CASC archive.
            std::string basePath;

            // The index table.
            insert_func inserter;

            /**
             * Registers block handlers.
             */
            template <typename T>
            void registerHandler()
            {
                if (Writeable)
                {
                    reinterpret_cast<write_buf_type*>(this->rdbuf())->registerHandler<T>();
                }
                else
                {
                    reinterpret_cast<read_buf_type*>(this->rdbuf())->registerHandler<T>();
                }
            }

            /**
             * Opens a file from the currently opened CASC file.
             */
            void open(size_t offset)
            {
                if (Writeable)
                {
                    auto buf = reinterpret_cast<write_buf_type*>(buffer.get());
                    if (buf->pubseekoff(offset, std::ios_base::beg, std::ios_base::out) != (std::streampos)offset)
                    {
                        throw Exceptions::IOException("Failed to open CASC archive for writing at given position.");
                    }
                }
                else
                {
                    auto buf = reinterpret_cast<read_buf_type*>(buffer.get());
                    buf->open(offset);
                }
            }

            /**
             * Opens a file in a CASC file.
             */
            void open(const char *filename, size_t offset)
            {
                if (Writeable)
                {
                    auto buf = reinterpret_cast<write_buf_type*>(buffer.get());
                    buf->open(filename, std::ios_base::out | std::ios_base::in | std::ios_base::binary);
                    if (buf->pubseekoff(offset, std::ios_base::beg, std::ios_base::out) != (std::streampos)offset)
                    {
                        throw Exceptions::IOException("Failed to open CASC archive for writing at given position.");
                    }
                }
                else
                {
                    auto buf = reinterpret_cast<read_buf_type*>(buffer.get());
                    buf->open(filename, offset);
                }
            }

            /**
             * Opens a file in a CASC file.
             */
            void open(const std::string filename, size_t offset)
            {
                open(filename.c_str(), offset);
            }

        public:
            /**
             * Constructor.
             */
            template <bool ReadConstructible = !Writeable,
                typename std::enable_if<ReadConstructible>::type* = nullptr>
            Stream(const std::string filename, size_t offset) :
                buffer(reinterpret_cast<buf_type*>(this->rdbuf())),
                base_type(new buf_type())
            {
                registerHandler<Impl::DefaultHandler>();
                registerHandler<Impl::ZlibHandler>();

                open(filename, offset);
            }

            template <bool WriteConstructible = Writeable,
                typename std::enable_if<WriteConstructible>::type* = nullptr>
            Stream(const std::string basePath,
                   insert_func inserter) :
                buffer(reinterpret_cast<buf_type*>(this->rdbuf())),
                basePath(basePath),
                base_type(new buf_type(basePath)),
                inserter(inserter)
            {
                registerHandler<Impl::DefaultHandler>();
                registerHandler<Impl::ZlibHandler>();
            }


            /**
            * Move constructor.
            */
            Stream(Stream &&) = default;

            /**
             * Destructor.
             */
            virtual ~Stream() = default;

            /**
             * Move operator.
             */
            Stream &operator= (Stream &&) = default;

            /**
             * Closes the currently opened CASC file.
             */
            void close()
            {
                if (Writeable)
                {
                    Parsers::Binary::Reference ref;
                    std::string encodingProfile;
                    Hex hash;
                    size_t size;

                    auto buf = reinterpret_cast<write_buf_type*>(buffer.get());
                    buf->close(ref, encodingProfile, hash, size);

                    if (inserter)
                        inserter(ref, hash, size);
                }
                this->rdbuf((buffer = std::make_unique<buf_type>(basePath)).get());
            }

            /**
             * Checks if the buffer is open.
             */
            bool is_open() const
            {
                return buffer->is_open();
            }

            /**
             * Sets the encoding mode for writeable streams.
             */
            void setMode(IO::EncodingMode mode)
            {
                if (Writeable)
                {
                    auto buf = reinterpret_cast<write_buf_type*>(buffer.get());
                    buf->setMode(mode);
                }
            }
        };
    }
}