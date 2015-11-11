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

#include <vector>

#include "../zlib.hpp"

namespace Casc
{
    namespace IO
    {
        /**
         * Types of data sources.
         */
        enum class DataSourceType
        {
            MemoryMapped,
            Stream
        };

        /**
         * A source for data.
         */
        class DataSource
        {
        public:
            /**
             * Constructor.
             */
            DataSource(DataSourceType type)
                : type(type) { }

            /**
             * Destructor.
             */
            virtual ~DataSource() { }

            /**
             * Gets a chunk of data.
             */
            virtual std::vector<char> get(size_t offset, size_t count) = 0;

            /**
             * The type of data source.
             */
            const DataSourceType type;
        };
    }
}

#include "Impl/MemoryMappedSource.hpp"
#include "Impl/StreamSource.hpp"