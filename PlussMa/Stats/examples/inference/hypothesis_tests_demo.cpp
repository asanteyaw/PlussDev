#include <pluss/ma.h>

#include <cstdint>
#include <iostream>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
namespace hyp = pluss::subsp::stats::inference::hypothesis;

Series make_vec(const std::vector<double>& values, const pluss::Config& cfg) {
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, cfg);
}

void print_result(const char* name, const hyp::TestDecision& r) {
  std::cout << name << ": stat=" << r.stat << ", p=" << r.pvalue
            << ", critical=" << r.critical << ", reject=" << (r.reject ? "true" : "false") << '\n';
}

} // namespace

int main() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series x1 = make_vec({4.0, 5.0, 6.0, 7.0, 8.0}, cfg);
  const Series y1 = make_vec({0.0, 1.0, 2.0, 3.0, 4.0}, cfg);

  hyp::TestOptions right;
  right.tail = hyp::Tail::Right;

  const auto t = hyp::ttest(x1, 3.0, right, cfg);
  const auto rs = hyp::ranksum(x1, y1, right, cfg);
  const auto sr = hyp::signrank(x1, 0.0, right, cfg);
  const auto st = hyp::signtest(x1, 0.0, right, cfg);

  const auto ks = hyp::kstest(x1, 0.0, 1.0, {}, cfg);
  const auto ks2 = hyp::kstest2(x1, y1, {}, cfg);
  const auto jb = hyp::jbtest(make_vec({0.1, 0.1, 0.2, 0.3, 0.5, 2.0, 4.0, 8.0, 16.0}, cfg), 0.05, cfg);
  const auto fr = hyp::f_ratio(x1, y1, right, cfg);

  const auto aov = hyp::anova_1w(
      make_vec({1.0, 1.2, 0.8, 3.1, 2.9, 3.0, 6.0, 6.2, 5.9}, cfg),
      make_vec({0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0}, cfg),
      {},
      cfg);

  const auto c2g = hyp::chisq_gof(
      make_vec({18.0, 21.0, 16.0, 25.0}, cfg),
      make_vec({20.0, 20.0, 20.0, 20.0}, cfg),
      {},
      cfg);

  const auto c2i = hyp::chisq_indep(
      pluss::make_series(
          std::vector<double>{
              90.0, 60.0, 104.0,
              30.0, 50.0, 51.0,
          },
          {2, 3},
          cfg),
      0.05,
      cfg);

  print_result("ttest", t);
  print_result("ranksum", rs);
  print_result("signrank", sr);
  print_result("signtest", st);
  print_result("kstest", ks);
  print_result("kstest2", ks2);
  print_result("jbtest", jb);
  print_result("f_ratio", fr);
  print_result("anova_1w", aov);
  print_result("chisq_gof", c2g);
  print_result("chisq_indep", c2i);

  return 0;
}
