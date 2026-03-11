#include "pluss/subsp/stats/inference/hypothesis_testing.h"

namespace pluss::subsp::stats::inference::hypothesis {

ArchLmResult arch_lm(const pluss::codem::Series& x,
                     const ArchLmOptions& options,
                     pluss::codem::Config cfg) {
  return ::pluss::subsp::stats::ts::arch_lm(x, options, cfg);
}

AdfResult adf_test(const pluss::codem::Series& y,
                   const AdfOptions& options,
                   pluss::codem::Config cfg) {
  return ::pluss::subsp::stats::ts::adf_test(y, options, cfg);
}

KpssResult kpss_test(const pluss::codem::Series& y,
                     const KpssOptions& options,
                     pluss::codem::Config cfg) {
  return ::pluss::subsp::stats::ts::kpss_test(y, options, cfg);
}

} // namespace pluss::subsp::stats::inference::hypothesis
