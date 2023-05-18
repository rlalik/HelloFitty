#ifndef FITTERFACTORY_PARSER_H
#define FITTERFACTORY_PARSER_H

#include "FitterFactory.h"

#include <memory>

class TString;

namespace FF::Tools
{
auto parseLineEntry_v1(const TString& line) -> std::unique_ptr<FF::HistogramFit>;
}

#endif /* FITTERFACTORY_PARSER_H */
