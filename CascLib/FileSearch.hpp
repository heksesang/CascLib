#pragma once

#include <filesystem>
#include <vector>
using namespace std::experimental::filesystem;

namespace Casc
{
	class FileSearch
	{
		std::vector<std::string> results_;

		void search(std::string filename, std::string path)
		{
			for (v1::directory_iterator iter(path), end; iter != end; ++iter)
			{
				auto current = iter->path();

				if (v1::is_directory(current))
				{
					search(filename, current.string());
				}
				else if (current.filename().compare(filename) == 0)
				{
					results_.push_back(current.string());
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