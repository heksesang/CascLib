#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "../CascLib/Shared/Utils.hpp"

namespace Casc
{
    using namespace Casc::Shared;

    /**
     * Class for parsing CASC configuration files.
     */
    class CascConfiguration
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
         * Default constructor.
         */
        CascConfiguration()
        {
        }

        /**
         * Constructor.
         *
         * @param path	the path of the configuration file.
         */
        CascConfiguration(std::string path)
        {
            parse(path);
        }

        /**
         * Destructor.
         */
        virtual ~CascConfiguration()
        {

        }

        /**
         * Gets the value for a key.
         *
         * @param key	the key to use for the lookup.
         * @return		the value.
         */
        const std::vector<std::string> &operator[] (const std::string& key) const
        {
            return values_.at(key);
        }

        /**
         * Clears old values and parses a configuration file.
         *
         * @param path	the path of the configuration file.
         */
        void parse(std::string path)
        {
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
                    fs >> value;
                    values_[key].push_back(value);
                    currentToken = TokenType::None;
                    break;
                }
            }

            fs.close();
        }
    };
}