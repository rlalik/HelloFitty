#ifndef HELLOFITTY_PARSER_H
#define HELLOFITTY_PARSER_H

#include "hellofitty.hpp"

#include <memory>

class TString;

#if __cplusplus < 201402L
template <typename T, typename... Args> std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#else
using std::make_unique;
#endif

namespace hf::parser
{
auto parse_line_entry_v1(const TString& line) -> std::unique_ptr<hf::fit_entry>;
auto format_line_entry_v1(const hf::fit_entry* hist_fit) -> TString;
} // namespace hf::parser

#endif /* HELLOFITTY_PARSER_H */
