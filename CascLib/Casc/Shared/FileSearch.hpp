#pragma once

#include <filesystem>
#include <vector>

namespace Casc
{
    using namespace std::experimental::filesystem;

    class FileSearch
    {
        std::vector<std::string> results_;

    public:
        FileSearch()
        {

        }

        FileSearch(const std::string &filename, const std::string &path)
        {
            search(filename, path);
        }

        FileSearch(std::initializer_list<std::string> files, const std::string &path)
        {
            for (auto &file : files)
            {
                search(file, path, false);
            }
        }

        const std::vector<std::string> &results() const
        {
            return results_;
        }

        void search(const std::string &filename, const std::string &path, bool clear = true)
        {
            if (clear)
                results_.clear();

            for (v1::directory_iterator iter(path), end; iter != end; ++iter)
            {
                auto current = iter->path();

                if (v1::is_directory(current))
                {
                    search(filename, current.string(), clear);
                }
                else if (current.filename().compare(filename) == 0)
                {
                    results_.push_back(current.string());
                }
            }
        }
    };
}