// One-sided tests and the significance level. `TestOptions::tail` chooses the alternative: Both (the mean differs), Right (the mean is larger than
// mean0) or Left (smaller). A one-sided test has more power against its alternative and none against the other. `alpha` is the significance
// level; `reject` is `pvalue < alpha`, and `critical` is the matching critical value of the statistic. Machine A against 5.0.
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
  const Series a = vec(machine_a);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << std::left << std::setw(10) << "tail" << std::setw(10) << "alpha" << std::setw(12) << "t" << std::setw(12) << "p-value" << std::setw(12) << "critical" << "decision\n";
  struct Case { const char* name; hyp::Tail tail; };
  const Case cases[] = {{"Both", hyp::Tail::Both}, {"Right", hyp::Tail::Right}, {"Left", hyp::Tail::Left}};
  for (const auto& c : cases) {
    for (double alpha : {0.05, 0.01}) {
      hyp::TestOptions o;
      o.tail = c.tail;
      o.alpha = alpha;
      const auto t = hyp::ttest(a, 5.0, o);
      std::cout << std::left << std::setw(10) << c.name << std::setw(10) << alpha << std::setw(12) << t.stat << std::setw(12) << t.pvalue << std::setw(12) << t.critical << yes_no(t.reject) << "\n";
    }
  }
}
