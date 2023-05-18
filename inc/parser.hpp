#ifndef FITEMALL_PARSER_H
#define FITEMALL_PARSER_H

#include "fitemall.hpp"

#include <memory>

class TString;

namespace fea::tools
{
auto parse_line_entry_v1(const TString& line) -> std::unique_ptr<fea::histogram_fit>;
}

#endif /* FITEMALL_PARSER_H */
