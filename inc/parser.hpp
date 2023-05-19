#ifndef HELLOFITTY_PARSER_H
#define HELLOFITTY_PARSER_H

#include "hellofitty.hpp"

#include <memory>

class TString;

namespace hf::parser
{
auto parse_line_entry_v1(const TString& line) -> std::unique_ptr<hf::histogram_fit>;
auto format_line_entry_v1(const hf::histogram_fit * hist_fit) -> TString;

}

#endif /* HELLOFITTY_PARSER_H */
