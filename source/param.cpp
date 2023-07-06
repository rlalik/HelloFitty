/*
    HelloFitty - a versatile histogram fitting tool for ROOT-based projects
    Copyright (C) 2015-2023  Rafał Lalik <rafallalik@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <fmt/core.h>

#include "hellofitty.hpp"

namespace hf
{

auto param::print() const -> void
{
    fmt::print("{:10g}   Mode: {:>5}   Limits: ", value, mode == fit_mode::free ? "free" : "fixed");
    if (has_limits)
        fmt::print(" {:g}, {:g}\n", min, max);
    else
        fmt::print(" none\n");
}

} // namespace hf
