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
/// Initial format with fixed two functions:
///  hist_name signal_func background_func rebin_flag range_min range_max param0 [... params]
/// @{
auto parse_line_entry_v1(const TString& line) -> std::unique_ptr<hf::fit_entry>;
auto format_line_entry_v1(const hf::fit_entry* hist_fit) -> TString;
/// @}

/// New format with variable number of functions:
///  hist_name range_min range_max rebin_flag function1 [... functions] | param0 [... params]
/// In order to separate functions from params a '|' marker is required
/// @{
auto parse_line_entry_v2(const TString& line) -> std::unique_ptr<hf::fit_entry>;
auto format_line_entry_v2(const hf::fit_entry* hist_fit) -> TString;
/// @}

} // namespace hf::parser

#endif /* HELLOFITTY_PARSER_H */
