#pragma once

#include <string>
#include <stdint.h>
#include "CascException.hpp"

namespace Casc
{
    namespace Exceptions
    {
        class FileNotFoundException : public CascException
        {
        public:
            FileNotFoundException(std::string key)
                : key(key)
            {

            }

            const std::string key;
        };
    }
}