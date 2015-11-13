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
#include "../../Hex.hpp"
#include "../Handler.hpp"

#include "../../IO/Endian.hpp"
#include "../../Crypto/Lookup3.hpp"

namespace Casc
{
    namespace Filesystem
    {
        namespace Impl
        {
            /**
             * Maps filename to file content MD5 hash. Uses lookup3.
             */
            class WoWHandler : public Handler
            {
                std::map<std::pair<uint32_t, uint32_t>, uint32_t> integers;
                std::map<std::pair<uint32_t, uint32_t>, Hex> checksums;

            public:
                /**
                 * Find the file content hash for the given filename.
                 */
                Hex findHash(std::string path) const override
                {
                    return checksums.at(Crypto::lookup3(path));
                };

            public:
                /**
                 * Default constructor.
                 */
                WoWHandler(std::vector<char> &data)
                {
                    for (auto it = data.begin(), end = data.end(); it < end;)
                    {
                        auto count = IO::Endian::read<IO::EndianType::Little, uint32_t, true>(it);
                        auto flags = IO::Endian::read<IO::EndianType::Little, uint32_t, true>(it);
                        auto locale = IO::Endian::read<IO::EndianType::Little, uint32_t, true>(it);

                        std::vector<std::pair<uint32_t, uint32_t>> hashes;
                        std::vector<uint32_t> integers;
                        std::vector<Hex> checksums;

                        std::map<uint32_t, uint32_t> counts;

                        for (auto i = 0U; i < count; ++i)
                        {
                            integers.push_back(IO::Endian::read<IO::EndianType::Little, uint32_t, true>(it));
                        }

                        for (auto i = 0U; i < count; ++i)
                        {
                            checksums.emplace_back(it, it + 16);
                            it += 16;

                            hashes.push_back(std::make_pair(
                                IO::Endian::read<IO::EndianType::Little, uint32_t, true>(it),
                                IO::Endian::read<IO::EndianType::Little, uint32_t, true>(it)));
                        }

                        for (auto i = 0U; i < count; i++)
                        {
                            this->integers[hashes[i]] = integers[i];
                            this->checksums[hashes[i]] = checksums[i];
                        }
                    }
                }

                using Handler::Handler;
            };
        }
    }
}