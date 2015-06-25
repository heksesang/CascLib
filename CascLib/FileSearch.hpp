#pragma once

#include <filesystem>
#include <vector>

namespace Casc
{
	using namespace std::tr2;

	class FileSearch
	{
		std::vector<std::string> results_;

		void search(std::string filename, std::string path)
		{
			for (sys::directory_iterator iter(path), end; iter != end; ++iter)
			{
				auto current = iter->path();

				if (sys::is_directory(current))
				{
					search(filename, current);
				}
				else if (current.filename().compare(filename) == 0)
				{
					results_.push_back(current);
				}
			}
		}

	public:
		FileSearch(std::string filename, std::string path)
		{
			search(filename, path);
		}

		const std::vector<std::string> &results() const
		{
			return results_;
		}
	};
}