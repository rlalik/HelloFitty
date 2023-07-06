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

#include "details.hpp"

namespace hf
{

draw_opts::draw_opts() : m_d{make_unique<detail::draw_opts_impl>()} {}

draw_opts::draw_opts(const draw_opts& other) : m_d{make_unique<detail::draw_opts_impl>(*other.m_d)} {}

auto draw_opts::set_visible(bool vis) -> draw_opts&
{
    m_d->visible = vis;
    return *this;
}

auto draw_opts::set_line_color(Color_t color) -> draw_opts&
{
    m_d->line_color = color;
    return *this;
}

auto draw_opts::set_line_width(Width_t width) -> draw_opts&
{
    m_d->line_width = width;
    return *this;
}

auto draw_opts::set_line_style(Style_t style) -> draw_opts&
{
    m_d->line_style = style;
    return *this;
}

auto draw_opts::apply(TF1* function) const -> void
{
#if __cplusplus >= 201703L
    if (m_d->visible)
    {
        if (m_d->visible == 0) { function->SetBit(TF1::kNotDraw); }
        else { function->ResetBit(TF1::kNotDraw); }
    }
    else { function->ResetBit(TF1::kNotDraw); }
    if (m_d->line_color) { function->SetLineColor(*m_d->line_color); }
    if (m_d->line_width) { function->SetLineWidth(*m_d->line_width); }
    if (m_d->line_style) { function->SetLineStyle(*m_d->line_style); }
#else
    if (m_d->visible != -1)
    {
        if (m_d->visible == 0) { function->SetBit(TF1::kNotDraw); }
        else { function->ResetBit(TF1::kNotDraw); }
    }
    else { function->ResetBit(TF1::kNotDraw); }
    if (m_d->line_color != -1) { function->SetLineColor(m_d->line_color); }
    if (m_d->line_width != -1) { function->SetLineWidth(m_d->line_width); }
    if (m_d->line_style != -1) { function->SetLineStyle(m_d->line_style); }
#endif
}

auto draw_opts::print() const -> void
{
    fmt::print(" STYLE INFO: Visible: {}  Color: {}  Width: {}  Style: {}\n",
#if __cplusplus >= 201703L
               m_d->visible.value_or(-999), m_d->line_color.value_or(-999), m_d->line_width.value_or(-999),
               m_d->line_style.value_or(-999)
#else
               m_d->visible, m_d->line_color, m_d->line_width, m_d->line_style
#endif
    );
}

} // namespace hf
