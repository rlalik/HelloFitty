#ifndef HELLOFITTY_PARSER_H
#define HELLOFITTY_PARSER_H

#include "HelloFitty/hellofitty_export.hpp"

#include "hellofitty.hpp"
#include "hellofitty_config.h"

#include "details.hpp"

#include <memory>

class TString;

namespace hf::parser
{

/// Initial format with fixed two functions:
///  hist_name signal_func background_func rebin_flag range_min range_max param0 [... params]
/// @{
struct v1
{
    static auto HELLOFITTY_EXPORT parse_line_entry(const TString& line) -> std::pair<TString, fit_entry>;
    static auto HELLOFITTY_EXPORT format_line_entry(const TString& name, const hf::fit_entry* hist_fit) -> TString;
};
/// @}

/// New format with variable number of functions:
///  hist_name range_min range_max rebin_flag function1 [... functions] | param0 [... params]
/// In order to separate functions from params a '|' marker is required
/// @{
struct v2
{
    static auto HELLOFITTY_EXPORT parse_line_entry(const TString& line) -> std::pair<TString, fit_entry>;
    static auto HELLOFITTY_EXPORT format_line_entry(const TString& name, const hf::fit_entry* hist_fit) -> TString;
};
/// @}

} // namespace hf::parser

#endif /* HELLOFITTY_PARSER_H */
