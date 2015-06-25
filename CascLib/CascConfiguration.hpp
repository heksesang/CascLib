#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "../CascLib/StringUtils.hpp"

namespace Casc
{
	class CascConfiguration
	{
		enum class TokenType
		{
			None,
			Comment,
			Key,
			Operator,
			Value
		};

		TokenType currentToken = TokenType::None;

		std::map<std::string, std::vector<std::string>> values_;

		void parse(std::string path)
		{
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
					key = String::trim(buffer);
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

	public:
		CascConfiguration(std::string path)
		{
			parse(path);
		}

		const std::vector<std::string> &operator[] (const std::string& key) const
		{
			return values_.at(key);
		}
	};
}