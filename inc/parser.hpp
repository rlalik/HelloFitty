#ifndef FITEMALL_PARSER_H
#define FITEMALL_PARSER_H

#include "fitemall.hpp"

#include <memory>

class TString;

namespace fea::parser
{
auto parse_line_entry_v1(const TString& line) -> std::unique_ptr<fea::histogram_fit>;
auto format_line_entry_v1(const fea::histogram_fit * hist_fit) -> TString;

}

#endif /* FITEMALL_PARSER_H */
