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

#include "Exceptions.hpp"

namespace Casc
{
    enum class ProgramCode
    {
        wow,
        wowt,
        wow_beta
    };

    ProgramCode getProgramCode(std::string str)
    {
        if (str == "wow")
        {
            return ProgramCode::wow;
        }
        else if (str == "wowt")
        {
            return ProgramCode::wowt;
        }
        else if (str == "wow_beta")
        {
            return ProgramCode::wow_beta;
        }

        throw Exceptions::CascException("Invalid progam code");
    }
}