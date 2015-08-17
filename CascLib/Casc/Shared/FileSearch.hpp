#pragma once

#ifdef _MSC_VER
#include <experimental/filesystem>
#else
#include <boost/filesystem.hpp>
#endif
#include <vector>

namespace Casc
{
#ifdef _MSC_VER
    namespace fs = std::experimental::filesystem::v1;
#else
    namespace fs = boost::filesystem;
#endif

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

            for (fs::directory_iterator iter(path), end; iter != end; ++iter)
            {
                auto current = iter->path();

                if (fs::is_directory(current))
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