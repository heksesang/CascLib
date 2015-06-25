#pragma once

#include <cctype>
#include <string>
#include <algorithm>

namespace Casc
{
	namespace String
	{
		inline std::string trim(const std::string &s)
		{
			auto wsfront = std::find_if_not(s.begin(), s.end(), std::isspace);
			auto wsback = std::find_if_not(s.rbegin(), s.rend(), std::isspace).base();
			return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
		}
	}
}