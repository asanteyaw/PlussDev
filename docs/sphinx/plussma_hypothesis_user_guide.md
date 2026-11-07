# PlussMa Hypothesis Testing User Guide

This guide covers `pluss::subsp::stats::inference::hypothesis` (also available as `pluss::ma::hyp`) with multiple ways to run each test family.

## 1) Includes And Namespaces

```cpp
#include <pluss/ma.h>                      // easiest umbrella
// or narrower:
#include <pluss/ma/stats/inference.h>

namespace hyp = pluss::ma::hyp;           // alias to hypothesis namespace
```

Deterministic baseline config:

```cpp
auto cfg = pluss::Config{}
  .with_dtype(pluss::ScalarType::Float64)
  .with_platform(pluss::pCPU);
```

Help notes:
- Most tests assume i.i.d. samples unless explicitly time-series-oriented.
- Start with `alpha=0.05`, then adjust for your domain and multiplicity plan.

## 2) Shared Option Types

### 2.1 Tail selection

```cpp
hyp::TestOptions two_sided{};                              // default tail=Both
hyp::TestOptions right{.alpha = 0.05, .tail = hyp::Tail::Right};
hyp::TestOptions left {.alpha = 0.01, .tail = hyp::Tail::Left};
```

### 2.2 Two-sample t-test options

```cpp
hyp::TTest2Options pooled{.alpha = 0.05, .tail = hyp::Tail::Both, .equal_variance = true};
hyp::TTest2Options welch {.alpha = 0.05, .tail = hyp::Tail::Both, .equal_variance = false};
```

## 3) Parametric Mean Tests

### 3.1 One-sample t-test

```cpp
auto r = hyp::ttest(x, /*mean0=*/0.0, hyp::TestOptions{.alpha = 0.05, .tail = hyp::Tail::Both}, cfg);
```

Alternative tails:

```cpp
auto r_right = hyp::ttest(x, 0.0, hyp::TestOptions{.tail = hyp::Tail::Right}, cfg);
auto r_left  = hyp::ttest(x, 0.0, hyp::TestOptions{.tail = hyp::Tail::Left}, cfg);
```

### 3.2 Paired t-test

```cpp
auto rp = hyp::ttest_paired(before, after, hyp::TestOptions{.tail = hyp::Tail::Both}, cfg);
```

### 3.3 Two-sample t-test (pooled vs Welch)

```cpp
auto r_pool = hyp::ttest2(x, y, hyp::TTest2Options{.equal_variance = true}, cfg);
auto r_welch = hyp::ttest2(x, y, hyp::TTest2Options{.equal_variance = false}, cfg);
```

## 4) Z-Test

```cpp
auto rz = hyp::ztest(x, /*mean0=*/0.0, /*sigma=*/1.2, hyp::TestOptions{.tail = hyp::Tail::Both}, cfg);
```

Help note:
- `ztest` assumes known population sigma; if sigma is estimated from sample, prefer `ttest`.

## 5) Variance Tests

### 5.1 One-sample variance test

```cpp
auto rv = hyp::vartest(x, /*variance0=*/1.0, hyp::TestOptions{.tail = hyp::Tail::Both}, cfg);
```

### 5.2 Two-sample variance ratio test

```cpp
auto rv2 = hyp::vartest2(x, y, hyp::TestOptions{.tail = hyp::Tail::Both}, cfg);
```

## 6) Rank/Sign Nonparametric Tests

### 6.1 Wilcoxon rank-sum (Mann-Whitney)

```cpp
auto rr = hyp::ranksum(x, y, hyp::TestOptions{.tail = hyp::Tail::Both}, cfg);
```

### 6.2 Signed-rank tests (one-sample and paired overloads)

```cpp
auto sr1 = hyp::signrank(x, /*median0=*/0.0, hyp::TestOptions{.tail = hyp::Tail::Both}, cfg);
auto sr2 = hyp::signrank(x, y, hyp::TestOptions{.tail = hyp::Tail::Both}, cfg);
```

### 6.3 Sign tests (one-sample and paired overloads)

```cpp
auto st1 = hyp::signtest(x, /*median0=*/0.0, hyp::TestOptions{.tail = hyp::Tail::Both}, cfg);
auto st2 = hyp::signtest(x, y, hyp::TestOptions{.tail = hyp::Tail::Both}, cfg);
```

Help notes:
- `signrank` is generally more powerful than `signtest` when symmetric differences are plausible.
- Use `signtest` when you want minimal assumptions and robustness.

## 7) Distributional Goodness/Normality Tests

### 7.1 One-sample KS test vs Normal(mean0, stddev0)

```cpp
auto ks = hyp::kstest(x, /*mean0=*/0.0, /*stddev0=*/1.0,
                      hyp::TestOptions{.alpha = 0.05, .tail = hyp::Tail::Both}, cfg);
```

### 7.2 Two-sample KS test

```cpp
auto ks2 = hyp::kstest2(x, y, hyp::TestOptions{.alpha = 0.05, .tail = hyp::Tail::Both}, cfg);
```

### 7.3 Jarque-Bera normality test

```cpp
auto jb = hyp::jbtest(x, /*alpha=*/0.05, cfg);
```

Help note:
- `jbtest` is moment-based (skewness + kurtosis), while KS compares empirical CDFs.

## 8) Time-Series Hypothesis Bridge (Same Namespace)

These are available in `hyp` and delegate to time-series implementations:

### 8.1 ARCH-LM

```cpp
auto a = hyp::arch_lm(residuals, hyp::ArchLmOptions{.lags = 8, .demean = true, .alpha = 0.05}, cfg);
```

### 8.2 ADF unit-root test

```cpp
auto adf = hyp::adf_test(y, hyp::AdfOptions{.lags = 1, .model = hyp::AdfModel::ARD, .alpha = 0.05}, cfg);
```

### 8.3 KPSS stationarity test

```cpp
auto kp = hyp::kpss_test(y, hyp::KpssOptions{.lags = 8, .trend = false, .alpha = 0.05}, cfg);
```

### 8.4 Diebold-Mariano forecast comparison test (`dm_test`)

Use this when comparing two forecast error streams directly.

```cpp
hyp::DmOptions dm_opt{};
dm_opt.horizon = 1;
dm_opt.loss = hyp::DmLoss::Squared;
dm_opt.tail = hyp::DmTail::Both;
dm_opt.alpha = 0.05;
dm_opt.hln_adjust = true;

auto dm = hyp::dm_test(err_model_a, err_model_b, dm_opt, cfg);
```

Alternative loss definitions:

```cpp
auto dm_abs = hyp::dm_test(
    err_model_a,
    err_model_b,
    hyp::DmOptions{
        .horizon = 1,
        .loss = hyp::DmLoss::Absolute,
        .tail = hyp::DmTail::Both,
        .alpha = 0.05,
    },
    cfg);

auto dm_pow = hyp::dm_test(
    err_model_a,
    err_model_b,
    hyp::DmOptions{
        .horizon = 1,
        .loss = hyp::DmLoss::Power,
        .loss_power = 1.5,
        .tail = hyp::DmTail::Right,
        .alpha = 0.05,
    },
    cfg);
```

Help note:
- ADF null is unit root; KPSS null is stationarity. Interpret jointly.
- DM null is equal predictive accuracy for the selected loss.

## 9) Reading Result Structs Properly

All result structs include at least:
- `stat`
- `pvalue`
- `critical`
- `reject`

Examples of additional fields:
- `TTestResult`: `dof`, `mean`, `std_err`, `ci`, `effective_n`
- `TTest2Result`: `equal_variance`, `effective_nx`, `effective_ny`, CI of mean difference
- `VarTest2Result`: `dof1`, `dof2`, CI of variance ratio
- `KSTestResult`: `d_plus`, `d_minus`
- `JBTestResult`: `skewness`, `kurtosis`

Quick print pattern:

```cpp
auto r = hyp::ttest(x, 0.0, {}, cfg);
std::cout << "stat=" << r.stat
          << " p=" << r.pvalue
          << " crit=" << r.critical
          << " reject=" << (r.reject ? "true" : "false")
          << " dof=" << r.dof << "\n";
```

## 10) Many Common Workflows

### 10.1 A/B test style (independent groups)

```cpp
auto mean_test = hyp::ttest2(group_a, group_b,
  hyp::TTest2Options{.equal_variance = false, .tail = hyp::Tail::Both}, cfg);

auto var_test = hyp::vartest2(group_a, group_b,
  hyp::TestOptions{.tail = hyp::Tail::Both}, cfg);
```

### 10.2 Before/after repeated measurements

```cpp
auto paired = hyp::ttest_paired(before, after, {}, cfg);
auto signed_rank = hyp::signrank(before, after, {}, cfg);
```

### 10.3 Robust median-centered checks

```cpp
auto sign_median = hyp::signtest(x, 0.0, {}, cfg);
auto rank_median = hyp::signrank(x, 0.0, {}, cfg);
```

### 10.4 Normality/shape diagnostics bundle

```cpp
auto jb = hyp::jbtest(x, 0.05, cfg);
auto ks = hyp::kstest(x, x.mean().item<double>(), x.std().item<double>(), {}, cfg);
```

## 11) Help Notes For Reliable Decisions

1. Tail direction must be chosen before looking at data results.
2. Statistical significance is not effect size; inspect means/CI/scale too.
3. For many simultaneous tests, apply multiplicity control in your workflow.
4. Check sample quality: NaNs/outliers/serial dependence can change inference.
5. For autocorrelated residuals, prefer time-series diagnostics (`arch_lm`, `adf_test`, `kpss_test`).

## 12) Typical Failure Cases And Fixes

1. Near-empty effective samples:
- Clean missing values and verify enough non-tied observations.

2. Wrong test family:
- Use paired tests only for matched samples.
- Use Welch (`equal_variance=false`) when variance equality is doubtful.

3. Misread null hypotheses:
- ADF and KPSS have opposite nulls.

4. Over-interpreting p-values:
- Report CI and practical magnitude alongside reject/non-reject.

## 13) Minimal End-To-End Example

```cpp
#include <pluss/ma.h>
#include <iostream>

int main() {
  namespace hyp = pluss::ma::hyp;
  auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  auto x = pluss::make_series(std::vector<double>{4.0,5.0,6.0,7.0,8.0}, {5}, cfg);
  auto y = pluss::make_series(std::vector<double>{0.0,1.0,2.0,3.0,4.0}, {5}, cfg);

  auto t = hyp::ttest(x, 3.0, hyp::TestOptions{.tail = hyp::Tail::Right}, cfg);
  auto rs = hyp::ranksum(x, y, hyp::TestOptions{.tail = hyp::Tail::Right}, cfg);
  auto ks2 = hyp::kstest2(x, y, {}, cfg);

  std::cout << "ttest p=" << t.pvalue << " reject=" << t.reject << "\n";
  std::cout << "ranksum p=" << rs.pvalue << " reject=" << rs.reject << "\n";
  std::cout << "kstest2 p=" << ks2.pvalue << " reject=" << ks2.reject << "\n";
  return 0;
}
```

## 14) Runnable Demo Source

- `PlussMa/Stats/examples/inference/hypothesis_tests_demo.cpp`
