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

#include "hellofitty.hpp"

#include "parser.hpp"

namespace hf
{

namespace tools
{

auto format_name(const std::string& name, const std::string& decorator) -> std::string
{
    TString str = decorator;
    str.ReplaceAll("*", name);
    return str.Data();
}

auto HELLOFITTY_EXPORT detect_format(const std::string& line) -> format_version
{
    if (line.find_first_of('|') == std::string::npos) { return hf::format_version::v1; }

    return hf::format_version::v2;
}

auto parse_line_entry(const std::string& line, format_version version) -> std::pair<std::string, entry>
{
    if (version == hf::format_version::detect) { version = tools::detect_format(line); }

    switch (version)
    {
        case format_version::v1:
            return parser::v1::parse_line_entry(line);
            break;
        case format_version::v2:
            return parser::v2::parse_line_entry(line);
            break;
        default:
            throw std::runtime_error("Parser not implemented");
            break;
    }
}

auto HELLOFITTY_EXPORT format_line_entry(const std::string& name, const hf::entry* entry,
                                         format_version version) -> std::string
{
    switch (version)
    {
        case format_version::v1:
            return parser::v1::format_line_entry(name, entry);
            break;
        case format_version::v2:
            return parser::v2::format_line_entry(name, entry);
            break;
        default:
            throw std::runtime_error("Parser not implemented");
            break;
    }
}

} // namespace tools

} // namespace hf
