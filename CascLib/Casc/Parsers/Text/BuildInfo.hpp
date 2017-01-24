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

#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../../Common.hpp"
#include "../../Exceptions.hpp"

namespace Casc
{
    namespace Parsers
    {
        namespace Text
        {
            /**
            * Parser for CASC .build.info files.
            */
            class BuildInfo
            {
                // The key structure.
                struct Key
                {
                    std::string name;
                    std::string type;
                    int length;
                };

                // Parser states.
                enum class State
                {
                    KeyBegin,
                    Key,
                    Type,
                    Size,
                    ValueBegin,
                    Value
                };

                // The parsed values.
                std::vector<std::map<std::string, std::string>> values;

            public:
                /**
                 * Constructor.
                 */
                BuildInfo(const std::string path)
                {
                    parse(path);
                }

                /**
                 * Destructor.
                 */
                virtual ~BuildInfo()
                {
                }

                /**
                * Gets the values for a build from the last parsed .build.info file.
                */
                const std::map<std::string, std::string> &build(int index) const
                {
                    return values.at(index);
                }

                /**
                 * Gets the number of values stored from the last parsed .build.info file.
                 */
                size_t size() const
                {
                    return values.size();
                }

                /**
                 * Clears old values and parses a .build.info file.
                 */
                void parse(const std::string path)
                {
                    if (!fs::exists(path))
                    {
                        throw Exceptions::FileNotFoundException(path);
                    }

                    values.clear();

                    State currentState = State::KeyBegin;

                    int bufferCurrent = -1;
                    int bufferSize = 0;
                    std::unique_ptr<char[]> buffer = nullptr;

                    std::ifstream fs;

                    fs.open(path, std::ios_base::in);
                    fs.seekg(0, std::ios_base::end);

                    bufferSize = (int)fs.tellg();
                    bufferSize += 1;

                    buffer = std::make_unique<char[]>(bufferSize);

                    fs.seekg(0, std::ios_base::beg);

                    std::vector<Key> keys;

                    int index = -1;

                    while (!fs.eof())
                    {
                        char ch = fs.get();
                        std::stringstream ss;

                        switch (currentState)
                        {
                        case State::KeyBegin:
                            currentState = State::Key;
                            buffer[++bufferCurrent] = ch;
                            break;

                        case State::Key:
                            switch (ch)
                            {
                            case '!':
                                currentState = State::Type;
                                buffer[++bufferCurrent] = '\0';
                                bufferCurrent = -1;

                                keys.push_back({ buffer.get() });
                                break;

                            default:
                                buffer[++bufferCurrent] = ch;
                                break;
                            }
                            break;

                        case State::Type:
                            switch (ch)
                            {
                            case ':':
                                currentState = State::Size;
                                buffer[++bufferCurrent] = '\0';
                                bufferCurrent = -1;

                                keys.back().type = buffer.get();
                                break;

                            default:
                                buffer[++bufferCurrent] = ch;
                                break;
                            }
                            break;

                        case State::Size:
                            switch (ch)
                            {
                            case '|':
                                currentState = State::KeyBegin;
                                buffer[++bufferCurrent] = '\0';
                                bufferCurrent = -1;

                                ss << buffer.get();
                                ss >> keys.back().length;

                                keys.back().type = buffer.get();
                                break;

                            case '\n':
                                currentState = State::ValueBegin;
                                buffer[++bufferCurrent] = '\0';
                                bufferCurrent = -1;

                                ss << buffer.get();
                                ss >> keys.back().length;

                                keys.back().type = buffer.get();
                                break;

                            default:
                                buffer[++bufferCurrent] = ch;
                                break;
                            }
                            break;

                        case State::ValueBegin:
                            if (fs.eof())
                            {
                                break;
                            }

                            currentState = State::Value;
                            buffer[++bufferCurrent] = ch;

                            values.resize(values.size() + 1);
                            break;

                        case State::Value:
                            switch (ch)
                            {
                            case '|':
                                buffer[++bufferCurrent] = '\0';
                                bufferCurrent = -1;

                                values.back()[keys[++index].name] = buffer.get();
                                break;

                            case '\n':
                                currentState = State::ValueBegin;
                                buffer[++bufferCurrent] = '\0';
                                bufferCurrent = -1;

                                values.back()[keys[++index].name] = buffer.get();

                                index = -1;
                                break;

                            default:
                                buffer[++bufferCurrent] = ch;
                                break;
                            }
                            break;
                        }
                    }

                    fs.close();
                }
            };
        }
    }
}