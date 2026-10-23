// Tests of a distribution. `hyp::kstest(x, mean0, stddev0)` compares the empirical distribution of x with the normal distribution with the given
// mean and standard deviation (the statistic is the largest gap between the two cumulative distributions; the null parameters must be fixed in
// advance, not estimated from x). `hyp::kstest2(x, y)` asks whether two samples come from the same distribution. `hyp::jbtest(x)` tests normality
// from the skewness and kurtosis (Jarque-Bera). `hyp::chisq_gof(observed, expected)` compares counts with expected counts. The Kolmogorov-Smirnov
// p-values are exact for small samples (up to 140 observations for one sample, nx * ny under 10000 for two); `TestOptions::method` overrides.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
Series table(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }
const char* yes_no(bool b) { return b ? "reject" : "keep"; }
// two samples used throughout: cup fill volumes (cl) from two filling machines
const std::vector<double> machine_a{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1};
const std::vector<double> machine_b{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3};
int main() {
  const Series a = vec(machine_a), b = vec(machine_b);
  std::cout << std::fixed << std::setprecision(4);
  const auto k1 = hyp::kstest(a, 5.5, 0.5);
  std::cout << "KS, A against N(5.5, 0.5^2)  : D = " << k1.stat << " (D+ " << k1.d_plus << ", D- " << k1.d_minus << "), p = " << k1.pvalue << ", " << yes_no(k1.reject) << "\n";
  hyp::TestOptions approx;
  approx.method = hyp::PValueMethod::Approximate;
  std::cout << "   with the Kolmogorov series: p = " << hyp::kstest(a, 5.5, 0.5, approx).pvalue << "  (the exact value is the right one at n = 8)\n";
  const auto k2 = hyp::kstest2(a, b);
  std::cout << "KS two-sample, A against B   : D = " << k2.stat << ", p = " << k2.pvalue << ", " << yes_no(k2.reject) << "\n";
  std::vector<double> sample;
  for (int i = 0; i < 30; ++i) sample.push_back(std::sin(i * 1.3) * 2.0 + (i % 5) * 0.3);
  const auto jb = hyp::jbtest(vec(sample));
  std::cout << "Jarque-Bera, 30 values       : JB = " << jb.stat << ", p = " << jb.pvalue << ", skewness " << jb.skewness << ", kurtosis " << jb.kurtosis << ", " << yes_no(jb.reject) << "\n";
  const auto chi = hyp::chisq_gof(vec({18, 22, 31, 29}), vec({25, 25, 25, 25}));
  std::cout << "chi-squared goodness of fit  : chi2 = " << chi.stat << " on " << chi.dof << " df, p = " << chi.pvalue << ", " << yes_no(chi.reject) << "  (a fair four-sided die?)\n";
}
