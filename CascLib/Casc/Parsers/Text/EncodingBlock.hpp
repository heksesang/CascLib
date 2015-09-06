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
#include <sstream>
#include <string.h>
#include <stdint.h>
#include <vector>

#include "../../Common.hpp"
#include "../../Exceptions.hpp"

namespace Casc
{
    namespace Parsers
    {
        namespace Text
        {
            using namespace Casc::Functions;

            class EncodingBlock
            {
                // The size of the block.
                size_t size_;

                // The block is "greedy".
                bool wildcard_;

                // The encoding mode of the block.
                IO::EncodingMode mode_;

                // The encoder parameters for the block.
                std::vector<std::string> params_;

                static void parseEncodingProfile(char *encodingProfile, char **encodingMode, char ***params, unsigned int *nParams)
                {
                    char *it;

                    for (it = encodingProfile; ; ++it)
                    {
                        if (!isWhitespace(*it))
                            break;
                    }

                    for (*encodingMode = it; *it; ++it)
                    {
                        if (*it == ':' || isWhitespace(*it))
                            break;
                    }

                    char *encodingModeEnd = it;

                    while (isWhitespace(*it))
                    {
                        ++it;
                    }

                    if (*it == '\0')
                    {
                        return;
                    }

                    if (*it != ':')
                        return;

                    *encodingModeEnd = '\0';

                    do
                    {
                        ++it;
                    } while (isWhitespace(*it));

                    if (*it == '{')
                    {
                        do
                        {
                            ++it;
                        } while (isWhitespace(*it));

                        *nParams = 1;

                        if (!*it)
                            return;

                        char *paramsStart = it;

                        int state = 1;
                        do
                        {
                            if (!isWhitespace(*it))
                            {
                                if (!state)
                                    return;

                                if (*it == '{')
                                {
                                    state++;
                                }
                                else
                                {
                                    if (*it == '}')
                                    {
                                        --state;
                                    }
                                    else
                                    {
                                        if (*it == ',' && state == 1)
                                            ++*nParams;
                                    }
                                }
                            }
                        } while (*++it);

                        if (state)
                            return;

                        char **paramArray = (char **)new char[(4 * (*nParams) | -((uint64_t)*nParams >> 30 != 0))];

                        if (*params)
                            delete[] * params;
                        *params = paramArray;

                        int paramIndex = 0;
                        **params = paramsStart;
                        for (int state = 1; *paramsStart; ++paramsStart)
                        {
                            if (*paramsStart != ' ' && *paramsStart != '\t' && *paramsStart != '\v' && *paramsStart != '\r' && *paramsStart != '\f' && *paramsStart != '\n')
                            {
                                if (!state)
                                    return;

                                if (*paramsStart == '{')
                                {
                                    ++state;
                                }
                                else if (*paramsStart == '}')
                                {
                                    --state;

                                    if (!state)
                                    {
                                        char *paramsEnd;
                                        for (paramsEnd = paramsStart; paramsEnd > (*params)[paramIndex]; --paramsEnd)
                                        {
                                            char ch = *(paramsEnd - 1);
                                            if (ch != ' ' && ch != '\t' && ch != '\v' && ch != '\r' && ch != '\f' && ch != '\n')
                                                break;
                                        }
                                        *paramsEnd = '\0';
                                    }
                                }
                                else if (*paramsStart == ',' && state == 1)
                                {
                                    char *paramsEnd = paramsStart;
                                    if (paramsStart > (*params)[paramIndex])
                                    {
                                        do
                                        {
                                            if (!isWhitespace(*(paramsEnd - 1)))
                                                break;
                                            --paramsEnd;
                                        } while (paramsEnd > (*params)[paramIndex]);
                                    }
                                    *paramsEnd = '\0';
                                    ++paramIndex;
                                    for (; *paramsStart; ++paramsStart)
                                    {
                                        if (!isWhitespace(paramsStart[1]))
                                            break;
                                    }
                                    (*params)[paramIndex] = paramsStart + 1;
                                }
                            }
                        }

                        if (paramIndex + 1 != *nParams)
                            abort();
                    }
                    else
                    {
                        *nParams = 1;

                        char **paramArray = (char **)new char[4];

                        if (*params)
                            delete[] * params;
                        *params = paramArray;

                        **params = it;

                        for (it = &it[strlen(it)]; it > **params; --it)
                        {
                            char ch = *(it - 1);
                            if (ch != ' ' && ch != '\t' && ch != '\v' && ch != '\r' && ch != '\f' && ch != '\n')
                                break;
                        }

                        *it = '\0';
                    }

                    if (*nParams == 1 && !***params)
                    {
                        *nParams = 0;

                        if (*params)
                            delete[] * params;
                        *params = 0;
                    }
                    else
                    {
                        return;
                    }

                    throw Exceptions::ParserException("Invalid encoding profile.");
                }

            public:
                EncodingBlock(size_t size, bool wildcard, IO::EncodingMode mode, std::vector<std::string> params)
                    : size_(size), wildcard_(wildcard), mode_(mode), params_(params)
                {

                }

                EncodingBlock(std::string input, std::vector<std::string> params)
                    : params_(params), wildcard_(false), size_(0), mode_(IO::EncodingMode::None)
                {
                    auto it = std::find(input.begin(), input.end(), '=');
                    this->mode_ = (IO::EncodingMode)*(it + 1);

                    std::string size(input.begin(), it);
                    std::vector<char> symbols{ 'M', 'K', '*' };
                    it = std::find_first_of(size.begin(), size.end(), symbols.begin(), symbols.end());

                    if (*input.begin() == '*')
                    {
                        wildcard_ = true;
                    }
                    else
                    {
                        std::stringstream ss;
                        ss << std::string(size.begin(), it);
                        ss >> this->size_;

                        for (; it != size.end(); ++it)
                        {
                            switch (*it)
                            {
                            case 'M':
                                this->size_ *= 1024 * 1024;
                                break;

                            case 'K':
                                this->size_ *= 1024;
                                break;

                            case '*':
                                wildcard_ = true;
                                break;
                            }
                        }
                    }
                }

                decltype(auto) size() const
                {
                    return size_;
                }

                decltype(auto) wildcard() const
                {
                    return wildcard_;
                }

                decltype(auto) mode() const
                {
                    return mode_;
                }

                decltype(auto) params() const
                {
                    return (params_);
                }

                static std::vector<EncodingBlock> parse(std::string profile)
                {
                    std::vector<EncodingBlock> v;

                    char* blockType = nullptr;
                    char** blocks = nullptr;
                    unsigned int blockCount = 0;

                    auto len = profile.length() + 1;
                    auto str = new char[len];
#if _MSC_VER
                    strcpy_s(str, len, profile.c_str());
#else
                    strcpy(str, profile.c_str());
#endif

                    parseEncodingProfile(str, &blockType, &blocks, &blockCount);

                    char* encType;
                    char** encParams;
                    unsigned int encParamCount;

                    for (auto i = 0U; i < blockCount; ++i)
                    {
                        std::vector<std::string> params;

                        encType = nullptr;
                        encParams = nullptr;
                        encParamCount = 0;
                        parseEncodingProfile(blocks[i], &encType, &encParams, &encParamCount);

                        for (auto j = 0U; j < encParamCount; ++j)
                        {
                            params.emplace_back(encParams[j]);
                        }

                        v.emplace_back(blocks[i], params);
                    }

                    return v;
                }
            };
        }
    }
}