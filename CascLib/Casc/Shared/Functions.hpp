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

#include <algorithm>
#include <array>
#include <bitset>
#include <cctype>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "../Exceptions.hpp"

#include "../lookup3.hpp"
#include "../md5.hpp"

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
                enum class EndianType
                {
                    Little,
                    Big
                };

                template <EndianType Type, typename T, typename InputIt>
                inline T read(InputIt first, InputIt last = InputIt(nullptr))
                {
                    using namespace Casc::Exceptions;

                    if (last == InputIt(nullptr))
                    {
                        last = first + sizeof(T);
                    }

                    if ((last - first) > sizeof(T))
                    {
                        throw GenericException("The iterators are not valid for this data type.");
                    }

                    T output{};
                    auto it = first;

                    typedef typename std::make_unsigned<typename std::iterator_traits<InputIt>::value_type>::type* unsigned_ptr;
                    typedef typename std::make_signed<typename std::iterator_traits<InputIt>::value_type>::type* signed_ptr;
                    
                    switch (Type)
                    {
                    case EndianType::Little:
                        for (it = first; it != last; ++it)
                        {
                            if (std::is_unsigned<T>::value)
                            {
                                output |= *reinterpret_cast<unsigned_ptr>(&*it) << (it - first) * 8;
                            }
                            else if (std::is_signed<T>::value)
                            {
                                output |= *reinterpret_cast<signed_ptr>(&*it) << (it - first) * 8;
                            }
                        }
                        break;

                    case EndianType::Big:
                        for (it = last - 1; it >= first; --it)
                        {
                            if (std::is_unsigned<T>::value)
                            {
                                output |= *reinterpret_cast<unsigned_ptr>(&*it) << ((sizeof(T) - 1) - (it - first)) * 8;
                            }
                            else if (std::is_signed<T>::value)
                            {
                                output |= *reinterpret_cast<signed_ptr>(&*it) << ((sizeof(T) - 1) - (it - first)) * 8;
                            }
                        }
                        break;
                    }

                    return output;
                }

                template <EndianType Type, typename T>
                inline std::array<char, sizeof(T)> write(T value)
                {
                    std::array<char, sizeof(T)> output{};
                    int i;

                    switch (Type)
                    {
                    case EndianType::Little:
                        for (i = 0; i < sizeof(T); ++i)
                        {
                            output[i] = (value >> i * 8) & 0xFF;
                        }
                        break;

                    case EndianType::Big:
                        for (i = 0; i < sizeof(T); ++i)
                        {
                            output[(sizeof(T) - 1) - i] = (value >> i * 8) & 0xFF;
                        }
                        break;
                    }

                    return output;
                }
            }

            namespace Hash
            {
                inline std::string md5(const std::string &str)
                {
                    return MD5(str).hexdigest();
                }

                template <typename Container>
                inline std::string md5(const Container &input)
                {
                    return MD5(input).hexdigest();
                }

                template <typename Container>
                inline std::pair<uint32_t, uint32_t> lookup3(const Container &container, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
                {
                    auto pc = init.first;
                    auto pb = init.second;

                    hashlittle2(
                        &*std::begin(container),
                        sizeof(typename Container::value_type) * (std::end(container) - std::begin(container)),
                        &pc, &pb);

                    return std::make_pair(pc, pb);
                }

                template <typename InputIt>
                inline std::pair<uint32_t, uint32_t> lookup3(InputIt first, InputIt last, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
                {
                    auto pc = init.first;
                    auto pb = init.second;

                    hashlittle2(
                        &*first,
                        sizeof(typename std::iterator_traits<InputIt>::value_type) * (last - first),
                        &pc, &pb);

                    return std::make_pair(pc, pb);
                }

                inline std::pair<uint32_t, uint32_t> lookup3(std::ifstream &stream, uint32_t length, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
                {
                    auto pc = init.first;
                    auto pb = init.second;
                    std::vector<char> buffer(length);

                    auto pos = stream.tellg();

                    stream.read(buffer.data(), length);
                    hashlittle2(buffer.data(), length, &pc, &pb);

                    stream.seekg(pos);

                    return std::make_pair(pc, pb);
                }
                
                template <typename Container>
                inline uint32_t lookup3(const Container &container, const uint32_t &init)
                {
                    return hashlittle(
                        &*std::begin(container),
                        sizeof(typename Container::value_type) * (std::end(container) - std::begin(container)),
                        init);
                }

                template <typename InputIt>
                inline uint32_t lookup3(InputIt first, InputIt last, const uint32_t &init)
                {
                    return hashlittle(
                        &*first,
                        sizeof(typename std::iterator_traits<InputIt>::value_type) * (last - first),
                        init);
                }

                inline uint32_t lookup3(std::ifstream &stream, uint32_t length, const uint32_t &init)
                {
                    auto hash = init;
                    std::vector<char> buffer(length);

                    auto pos = stream.tellg();

                    stream.read(buffer.data(), length);
                    hash = hashlittle(buffer.data(), length, hash);

                    stream.seekg(pos);

                    return hash;
                }
            }
        }
    }
}