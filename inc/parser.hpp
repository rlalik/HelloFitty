#ifndef HELLOFITTY_PARSER_H
#define HELLOFITTY_PARSER_H

#include "hellofitty.hpp"

#include <memory>

class TString;

namespace hf::parser
{
auto parse_line_entry_v1(const TString& line) -> std::unique_ptr<hf::fit_entry>;
auto format_line_entry_v1(const hf::fit_entry* hist_fit) -> TString;
} // namespace hf::parser

#endif /* HELLOFITTY_PARSER_H */
