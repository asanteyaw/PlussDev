#pragma once

#include "pluss/subsp/stats/ts.h"

namespace pluss::subsp::stats::inference::hypothesis {

using ArchLmOptions = ::pluss::subsp::stats::ts::ArchLmOptions;
using ArchLmResult = ::pluss::subsp::stats::ts::ArchLmResult;

using AdfModel = ::pluss::subsp::stats::ts::AdfModel;
using AdfOptions = ::pluss::subsp::stats::ts::AdfOptions;
using AdfResult = ::pluss::subsp::stats::ts::AdfResult;

using KpssOptions = ::pluss::subsp::stats::ts::KpssOptions;
using KpssResult = ::pluss::subsp::stats::ts::KpssResult;

CODEM_API ArchLmResult arch_lm(const pluss::codem::Series& x,
                               const ArchLmOptions& options = {},
                               pluss::codem::Config cfg = {});

CODEM_API AdfResult adf_test(const pluss::codem::Series& y,
                             const AdfOptions& options = {},
                             pluss::codem::Config cfg = {});

CODEM_API KpssResult kpss_test(const pluss::codem::Series& y,
                               const KpssOptions& options = {},
                               pluss::codem::Config cfg = {});

} // namespace pluss::subsp::stats::inference::hypothesis
