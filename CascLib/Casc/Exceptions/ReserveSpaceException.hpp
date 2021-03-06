/*
* Copyright 2015 Gunnar Lilleaasen
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

#include <stdint.h>
#include "CascException.hpp"

namespace Casc
{
    namespace Exceptions
    {
        class ReserveSpaceException : public CascException
        {
        public:
            ReserveSpaceException(uint32_t requested, uint32_t available)
                : requested(requested), available(available), CascException("Couldn't find enough free space.")
            {

            }

            const uint32_t requested;
            const uint32_t available;
        };
    }
}