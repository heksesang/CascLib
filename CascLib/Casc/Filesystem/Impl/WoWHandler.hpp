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

#include <string>
#include <memory>
#include <map>
#include <stdint.h>
#include <array>
#include <fstream>

#include "../../Common.hpp"
#include "../Handler.hpp"

namespace Casc
{
    namespace Filesystem
    {
        /**
        * Maps filename to file content MD5 hash.
        *
        * TODO: Finish implementation, need different handling per game.
        */
        class WoWHandler : public Handler
        {
        public:
            /**
            * Find the file content hash for the given filename.
            *
            * @param filename  the filename.
            * @return          the hash in hex format.
            */
            std::string findHash(std::string filename) const override
            {
                return "";
            };

        public:
            /**
            * Default constructor.
            */
            WoWHandler(std::vector<char> &data)
            {

            }

            /**
            * The file magic of the root file.
            */
            static constexpr uint32_t Signature()
            {
                return 0x1967;
            }

            using Handler::Handler;
        };
    }
}