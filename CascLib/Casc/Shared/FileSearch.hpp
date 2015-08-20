/*
* Copyright 2014 Gunnar Lilleaasen
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