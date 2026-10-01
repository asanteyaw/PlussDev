#pragma once

#include "pluss/subsp/stats/ts.h"

#include <cstdint>

namespace pluss::subsp::stats::inference::hypothesis {

enum class Tail : std::uint8_t {
  Both = 0,
  Left = 1,
  Right = 2,
};

struct TestOptions {
  double alpha = 0.05;
  Tail tail = Tail::Both;
};

struct TTest2Options {
  double alpha = 0.05;
  Tail tail = Tail::Both;
  bool equal_variance = true;
};

struct TestDecision {
  double stat = 0.0;
  double pvalue = 1.0;
  double critical = 0.0;
  bool reject = false;
};

struct TTestResult : TestDecision {
  double dof = 0.0;
  double mean = 0.0;
  double null_mean = 0.0;
  double stderr = 0.0;
  pluss::codem::Series ci = pluss::series::undefined(); // [2]
  std::int64_t effective_n = 0;
};

struct TTest2Result : TestDecision {
  double dof = 0.0;
  double mean_x = 0.0;
  double mean_y = 0.0;
  double stderr = 0.0;
  bool equal_variance = true;
  pluss::codem::Series ci = pluss::series::undefined(); // [2], CI of mean_x - mean_y
  std::int64_t effective_nx = 0;
  std::int64_t effective_ny = 0;
};

struct ZTestResult : TestDecision {
  double mean = 0.0;
  double null_mean = 0.0;
  double sigma = 1.0;
  double stderr = 0.0;
  pluss::codem::Series ci = pluss::series::undefined(); // [2]
  std::int64_t effective_n = 0;
};

struct VarTestResult : TestDecision {
  double dof = 0.0;
  double sample_variance = 0.0;
  double null_variance = 0.0;
  pluss::codem::Series ci = pluss::series::undefined(); // [2], CI of variance
  std::int64_t effective_n = 0;
};

struct VarTest2Result : TestDecision {
  double dof1 = 0.0;
  double dof2 = 0.0;
  double var_x = 0.0;
  double var_y = 0.0;
  pluss::codem::Series ci = pluss::series::undefined(); // [2], CI of var_x / var_y
  std::int64_t effective_nx = 0;
  std::int64_t effective_ny = 0;
};

struct RankSumResult : TestDecision {
  double rank_sum_x = 0.0;
  double u_stat = 0.0;
  std::int64_t effective_nx = 0;
  std::int64_t effective_ny = 0;
};

struct SignRankResult : TestDecision {
  double wplus = 0.0;
  double wminus = 0.0;
  std::int64_t effective_n = 0;
};

struct SignTestResult : TestDecision {
  std::int64_t positives = 0;
  std::int64_t negatives = 0;
  std::int64_t effective_n = 0;
};

struct KSTestResult : TestDecision {
  double d_plus = 0.0;
  double d_minus = 0.0;
  double mean0 = 0.0;
  double stddev0 = 1.0;
  std::int64_t effective_n = 0;
};

struct KSTest2Result : TestDecision {
  double d_plus = 0.0;
  double d_minus = 0.0;
  std::int64_t effective_nx = 0;
  std::int64_t effective_ny = 0;
};

struct JBTestResult : TestDecision {
  double skewness = 0.0;
  double kurtosis = 3.0;
  std::int64_t effective_n = 0;
};

struct Anova1WOptions {
  double alpha = 0.05;
  bool drop_missing = true;
};

struct Anova1WResult : TestDecision {
  double dof_between = 0.0;
  double dof_within = 0.0;
  double ss_between = 0.0;
  double ss_within = 0.0;
  double ms_between = 0.0;
  double ms_within = 0.0;
  std::int64_t groups = 0;
  std::int64_t effective_n = 0;
  pluss::codem::Series group_means = pluss::series::undefined(); // [k]
  pluss::codem::Series group_counts = pluss::series::undefined(); // [k]
};

struct ChiGofOptions {
  double alpha = 0.05;
  double ddof = 0.0;
  bool normalize_expected = true;
};

struct ChiGofResult : TestDecision {
  double dof = 0.0;
  double total_count = 0.0;
  std::int64_t bins = 0;
  pluss::codem::Series expected = pluss::series::undefined(); // [k]
};

struct ChiIndepResult : TestDecision {
  double dof = 0.0;
  std::int64_t rows = 0;
  std::int64_t cols = 0;
  pluss::codem::Series expected = pluss::series::undefined(); // [r,c]
};

using ArchLmOptions = ::pluss::subsp::stats::ts::ArchLmOptions;
using ArchLmResult = ::pluss::subsp::stats::ts::ArchLmResult;

using AdfModel = ::pluss::subsp::stats::ts::AdfModel;
using AdfOptions = ::pluss::subsp::stats::ts::AdfOptions;
using AdfResult = ::pluss::subsp::stats::ts::AdfResult;

using KpssOptions = ::pluss::subsp::stats::ts::KpssOptions;
using KpssResult = ::pluss::subsp::stats::ts::KpssResult;

using DmLoss = ::pluss::subsp::stats::ts::DmLoss;
using DmTail = ::pluss::subsp::stats::ts::DmTail;
using DmOptions = ::pluss::subsp::stats::ts::DmOptions;
using DmResult = ::pluss::subsp::stats::ts::DmResult;

CODEM_API ArchLmResult arch_lm(const pluss::codem::Series& x,
                               const ArchLmOptions& options = {},
                               pluss::codem::Config cfg = {});

CODEM_API AdfResult adf_test(const pluss::codem::Series& y,
                             const AdfOptions& options = {},
                             pluss::codem::Config cfg = {});

CODEM_API KpssResult kpss_test(const pluss::codem::Series& y,
                               const KpssOptions& options = {},
                               pluss::codem::Config cfg = {});

CODEM_API DmResult dm_test(const pluss::codem::Series& err_a,
                           const pluss::codem::Series& err_b,
                           const DmOptions& options = {},
                           pluss::codem::Config cfg = {});

CODEM_API TTestResult ttest(const pluss::codem::Series& x,
                            double mean0 = 0.0,
                            const TestOptions& options = {},
                            pluss::codem::Config cfg = {});

CODEM_API TTestResult ttest_paired(const pluss::codem::Series& x,
                                   const pluss::codem::Series& y,
                                   const TestOptions& options = {},
                                   pluss::codem::Config cfg = {});

CODEM_API TTest2Result ttest2(const pluss::codem::Series& x,
                              const pluss::codem::Series& y,
                              const TTest2Options& options = {},
                              pluss::codem::Config cfg = {});

CODEM_API ZTestResult ztest(const pluss::codem::Series& x,
                            double mean0,
                            double sigma,
                            const TestOptions& options = {},
                            pluss::codem::Config cfg = {});

CODEM_API VarTestResult vartest(const pluss::codem::Series& x,
                                double variance0,
                                const TestOptions& options = {},
                                pluss::codem::Config cfg = {});

CODEM_API VarTest2Result vartest2(const pluss::codem::Series& x,
                                  const pluss::codem::Series& y,
                                  const TestOptions& options = {},
                                  pluss::codem::Config cfg = {});

CODEM_API RankSumResult ranksum(const pluss::codem::Series& x,
                                const pluss::codem::Series& y,
                                const TestOptions& options = {},
                                pluss::codem::Config cfg = {});

CODEM_API SignRankResult signrank(const pluss::codem::Series& x,
                                  double median0 = 0.0,
                                  const TestOptions& options = {},
                                  pluss::codem::Config cfg = {});

CODEM_API SignRankResult signrank(const pluss::codem::Series& x,
                                  const pluss::codem::Series& y,
                                  const TestOptions& options = {},
                                  pluss::codem::Config cfg = {});

CODEM_API SignTestResult signtest(const pluss::codem::Series& x,
                                  double median0 = 0.0,
                                  const TestOptions& options = {},
                                  pluss::codem::Config cfg = {});

CODEM_API SignTestResult signtest(const pluss::codem::Series& x,
                                  const pluss::codem::Series& y,
                                  const TestOptions& options = {},
                                  pluss::codem::Config cfg = {});

CODEM_API KSTestResult kstest(const pluss::codem::Series& x,
                              double mean0 = 0.0,
                              double stddev0 = 1.0,
                              const TestOptions& options = {},
                              pluss::codem::Config cfg = {});

CODEM_API KSTest2Result kstest2(const pluss::codem::Series& x,
                                const pluss::codem::Series& y,
                                const TestOptions& options = {},
                                pluss::codem::Config cfg = {});

CODEM_API JBTestResult jbtest(const pluss::codem::Series& x,
                              double alpha = 0.05,
                              pluss::codem::Config cfg = {});

CODEM_API Anova1WResult anova_1w(const pluss::codem::Series& y,
                                 const pluss::codem::Series& groups,
                                 const Anova1WOptions& options = {},
                                 pluss::codem::Config cfg = {});

CODEM_API VarTest2Result f_ratio(const pluss::codem::Series& x,
                                 const pluss::codem::Series& y,
                                 const TestOptions& options = {},
                                 pluss::codem::Config cfg = {});

CODEM_API ChiGofResult chisq_gof(const pluss::codem::Series& observed,
                                 const pluss::codem::Series& expected = pluss::series::undefined(),
                                 const ChiGofOptions& options = {},
                                 pluss::codem::Config cfg = {});

CODEM_API ChiIndepResult chisq_indep(const pluss::codem::Series& contingency,
                                     double alpha = 0.05,
                                     pluss::codem::Config cfg = {});

} // namespace pluss::subsp::stats::inference::hypothesis
