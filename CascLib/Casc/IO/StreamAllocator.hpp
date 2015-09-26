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
#include <memory>
#include <sstream>

#include "../Common.hpp"

#include "../Parsers/Binary/Reference.hpp"
#include "Stream.hpp"

namespace Casc
{
    namespace IO
    {
        class StreamAllocator
        {
            std::string basePath;

        public:
            StreamAllocator(const std::string basePath)
                : basePath(basePath)
            {

            }

            template <bool Writeable>
            typename std::enable_if<!Writeable, std::shared_ptr<Stream<Writeable>>>::type
                allocate(const Parsers::Binary::Reference &ref) const
            {
                std::stringstream ss;

                ss << basePath << PathSeparator
                   << "data." << std::setw(3) << std::setfill('0') << ref.file();
                
                return std::make_shared<Stream<Writeable>>(ss.str(), ref.offset());
            }

            template <bool Writeable>
            typename std::enable_if<Writeable, std::shared_ptr<Stream<Writeable>>>::type
                 allocate(typename Stream<Writeable>::insert_func inserter) const
            {
                return std::make_shared<Stream<Writeable>>(basePath, inserter);
            }
        };
    }
}