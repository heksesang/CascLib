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

#include "Buffer.hpp"

namespace Casc
{
    namespace IO
    {
        /**
         * 
         */
        class Stream : public std::istream
        {
        private:
            // The underlying buffer.
            std::unique_ptr<Buffer> buf;

        public:
            /**
             * Constructor.
             */
            Stream() :
                buf(reinterpret_cast<Buffer*>(this->rdbuf())),
                std::istream(new Buffer()) { }

            /**
             * Constructor.
             */
            Stream(const std::string filename, size_t offset) :
                buf(reinterpret_cast<Buffer*>(this->rdbuf())),
                std::istream(new Buffer())
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
            virtual ~Stream() = default;

            /**
             * Move operator.
             */
            Stream &operator= (Stream &&) = default;

            /**
             * Opens a file.
             */
            void open(const char *filename, size_t offset)
            {
                buf->open(filename, offset);
            }

            /**
             * Opens a file.
             */
            void open(const std::string filename, size_t offset)
            {
                open(filename.c_str(), offset);
            }

            /**
             * Closes the file.
             */
            void close()
            {
                this->rdbuf((buf = std::make_unique<Buffer>()).get());
            }

            /**
             * Checks if the stream is open.
             */
            bool is_open() const
            {
                return buf->is_open();
            }
        };
    }
}