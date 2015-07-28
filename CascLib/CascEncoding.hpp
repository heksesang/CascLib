#pragma once

#include <fstream>
#include <string>

namespace Casc
{
    class CascEncoding
    {
    public:
    private:
    public:
        CascEncoding(std::string path)
        {
            std::ifstream fs;
            fs.open(path, std::ios_base::in | std::ios_base::binary);

            char magic[2];
            fs.read(magic, 2);

            if (magic[0] != 0x45 || magic[1] != 0x4E)
            {
                throw std::exception((std::string("Invalid file magic in ") + path).c_str());
            }

            fs.seekg(1, std::ios::cur);


        }

        CascEncoding(std::ifstream stream)
        {

        }
    };
}