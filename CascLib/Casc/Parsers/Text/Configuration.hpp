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
            using namespace Casc::Functions;

            /**
             * Parser for CASC configuration files.
             */
            class Configuration
            {
                // The token types that can be parsed.
                enum class TokenType
                {
                    None,
                    Comment,
                    Key,
                    Operator,
                    Value
                };

                // The type of the token currently being parsed.
                TokenType currentToken = TokenType::None;

                // The parsed values.
                std::map<std::string, std::vector<std::string>> values_;

            public:
                /**
                 * Constructor.
                 */
                Configuration(const std::string path)
                {
                    parse(path);
                }

                /**
                 * Destructor.
                 */
                virtual ~Configuration()
                {

                }

                /**
                 * Gets the value for a key.
                 */
                const std::vector<std::string> &operator[] (const std::string key) const
                {
                    return values_.at(key);
                }

                /**
                 * Clears old values and parses a configuration file.
                 */
                void parse(const std::string path)
                {
                    if (!fs::exists(path))
                    {
                        throw Exceptions::FileNotFoundException(path);
                    }

                    values_.clear();

                    std::ifstream fs;
                    fs.open(path, std::ios_base::in);

                    char ch{};
                    char buffer[256]{};

                    std::string key;
                    std::string value;

                    while (!fs.eof())
                    {
                        switch (currentToken)
                        {
                        case TokenType::None:
                            ch = fs.peek();

                            switch (ch)
                            {
                            case '#':
                                currentToken = TokenType::Comment;
                                break;

                            case '\n':
                                fs.get();
                                break;

                            case ' ':
                                fs.get();
                                currentToken = TokenType::Value;
                                break;

                            default:
                                currentToken = TokenType::Key;
                                break;
                            }
                            break;

                        case TokenType::Comment:
                            fs.getline(buffer, 256);
                            currentToken = TokenType::None;
                            break;

                        case TokenType::Key:
                            fs.getline(buffer, 256, '=');
                            key = trim(buffer);
                            currentToken = TokenType::Operator;
                            break;

                        case TokenType::Operator:
                            ch = fs.get();
                            currentToken = TokenType::Value;
                            break;

                        case TokenType::Value:
                            std::getline(fs, value);
                            std::istringstream ss(value);
                            values_[key] = std::vector<std::string>
                            {
                                std::istream_iterator<std::string>{ss},
                                std::istream_iterator<std::string>{}
                            };
                            currentToken = TokenType::None;
                            break;
                        }
                    }

                    fs.close();
                }
            };
        }
    }
}