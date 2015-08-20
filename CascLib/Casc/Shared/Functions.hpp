/*
* Copyright 2014 Gunnar Lilleaasen
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

#include <cctype>
#include <string>
#include <algorithm>
#include <bitset>
#include <array>
#include <fstream>
#include <vector>
#include <utility>

namespace Casc
{
    namespace Shared
    {
        namespace Functions
        {
            template <typename M>
            std::vector<typename M::mapped_type> mapToVector(const  M &m) {
                std::vector<typename M::mapped_type> v;

                for (typename M::const_iterator it = m.begin(); it != m.end(); ++it) {
                    v.push_back(it->second);
                }

                return std::move(v);
            }

            inline std::string trim(const std::string &s)
            {
                int (*pred)(int c) = std::isspace;
                auto wsfront = std::find_if_not(s.begin(), s.end(), pred);
                auto wsback = std::find_if_not(s.rbegin(), s.rend(), pred).base();
                return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
            }

            namespace Endian
            {
                template <typename T>
                inline T readLE(unsigned char* arr)
                {
                    T output{};

                    for (int i = 0; i < sizeof(T); ++i)
                    {
                        output |= arr[i] << i * 8;
                    }

                    return output;
                }

                template <typename T>
                inline T readLE(char* arr)
                {
                    return readLE<T>(reinterpret_cast<unsigned char*>(arr));
                }

                template <typename T>
                inline T readLE(T value)
                {
                    return readLE<T>(reinterpret_cast<unsigned char*>(&value));
                }

                template <typename T>
                inline T readBE(unsigned char* arr)
                {
                    T output{};

                    for (int i = sizeof(T) - 1; i >= 0; --i)
                    {
                        output |= arr[i] << ((sizeof(T) - 1) - i) * 8;
                    }

                    return output;
                }

                template <typename T>
                inline T readBE(char* arr)
                {
                    return readBE<T>(reinterpret_cast<unsigned char*>(arr));
                }

                template <typename T>
                inline T readBE(T value)
                {
                    return readBE<T>(reinterpret_cast<unsigned char*>(&value));
                }
            }

            namespace Extern
            {
                extern "C"
                {
                    void hashlittle2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);
                    uint32_t hashlittle(const void *key, size_t length, uint32_t initval);
                }
            }

            namespace Hash
            {
                using namespace Casc::Shared::Functions::Extern;

                inline std::pair<uint32_t, uint32_t> lookup3(const std::string &data, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
                {
                    auto pc = init.first;
                    auto pb = init.second;

                    hashlittle2(data.c_str(), data.size() + 1, &pc, &pb);

                    return std::make_pair(pc, pb);
                }

                inline std::pair<uint32_t, uint32_t> lookup3(const std::vector<char> &data, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
                {
                    auto pc = init.first;
                    auto pb = init.second;

                    hashlittle2(data.data(), data.size(), &pc, &pb);

                    return std::make_pair(pc, pb);
                }

                inline std::pair<uint32_t, uint32_t> lookup3(std::ifstream &stream, uint32_t length, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
                {
                    auto pc = init.first;
                    auto pb = init.second;
                    std::array<char, 12> buffer;

                    for (unsigned int i = 0; i < length; i += 12)
                    {
                        stream.read(buffer.data(), buffer.size());
                        hashlittle2(buffer.data(), (size_t)stream.gcount(), &pc, &pb);
                    }

                    return std::make_pair(pc, pb);
                }

                inline uint32_t lookup3(const std::string &data, const uint32_t &init)
                {
                    return hashlittle(data.c_str(), data.size() + 1, init);
                }

                inline uint32_t lookup3(const std::vector<char> &data, const uint32_t &init)
                {
                    return hashlittle(data.data(), data.size(), init);
                }

                inline uint32_t lookup3(std::ifstream &stream, uint32_t length, const uint32_t &init)
                {
                    auto hash = init;
                    std::array<char, 12> buffer;

                    auto pos = stream.tellg();

                    for (unsigned int i = 0; i < length; i += 12)
                    {
                        stream.read(buffer.data(), buffer.size());
                        hash = hashlittle(buffer.data(), (size_t)stream.gcount(), hash);
                    }

                    stream.seekg(pos);

                    return hash;
                }
            }
        }
    }
}