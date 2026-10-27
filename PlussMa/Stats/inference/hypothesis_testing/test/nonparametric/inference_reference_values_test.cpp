// Reference values for three conventions of the rank / distribution tests, each computed independently of this library
// (two statistical packages agree on every number):
//   - kstest2 evaluates both empirical distribution functions at the pooled values, so repeated values count together;
//   - the normal approximation of signrank carries no continuity correction (ranksum keeps its correction);
//   - jbtest takes its p-value from the simulated finite-sample distribution, not the chi-squared(2) limit.
#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace hyp = pluss::subsp::stats::inference::hypothesis;

void expect(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

void expect_near(double got, double want, double tol, const std::string& what) {
  if (!(std::fabs(got - want) <= tol)) throw std::runtime_error(what + ": got " + std::to_string(got) + ", want " + std::to_string(want));
}

pluss::Series vec(const std::vector<double>& v) {
  return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
}

}  // namespace

int main() {
  try {
    const std::vector<double> a = {0.4377, -0.2434, 1.6001, 0.8548, 0.1584, 0.7118, 0.1149, -0.0624, -0.4544, -1.1883, -2.2031, -0.3908, -0.9935, 0.4239, 0.5146, -2.3045, 0.9449, 1.7149, 0.6289, 1.0732, -1.3953, 1.6657, 0.5799, 2.0664, 0.0056};
    const std::vector<double> s1 = {-0.215, 2.985, 0.815, 0.984, -1.157, 0.669, -1.462, 1.139};
    const std::vector<double> t1 = {3.0, 5.0, 5.0, 7.0, 8.0, 8.0, 8.0, 10.0, 12.0, 12.0, 14.0, 15.0};
    const std::vector<double> t2 = {4.0, 5.0, 6.0, 8.0, 8.0, 9.0, 11.0, 11.0, 13.0, 13.0, 13.0, 16.0, 18.0, 20.0};
    const std::vector<double> z = {0.5, -0.2, 0.0, 1.3, 0.0, 2.1, -0.7, 0.9, 1.8, 0.4, -1.1, 0.6};

    // Two samples with repeated values: the distance is 0.2619..., not the 0.2976... an index-based step gives.
    {
      const auto r = hyp::kstest2(vec(t1), vec(t2));
      expect_near(r.stat, 0.2619047619047619, 1e-12, "kstest2 statistic with ties");
      expect_near(r.pvalue, 0.6843530033030638, 1e-9, "kstest2 exact p-value with ties");
    }

    // Normal approximation of the signed-rank test, no continuity correction.
    {
      hyp::TestOptions o;
      o.method = hyp::PValueMethod::Approximate;
      o.tail = hyp::Tail::Left;
      expect_near(hyp::signrank(vec(s1), 0.0, o).pvalue, 0.7122984885436232, 1e-9, "signrank left, approximate");
      o.tail = hyp::Tail::Right;
      expect_near(hyp::signrank(vec(s1), 0.0, o).pvalue, 0.2877015114563768, 1e-9, "signrank right, approximate");
      o.tail = hyp::Tail::Both;
      expect_near(hyp::signrank(vec(a), 0.0, o).pvalue, 0.3002412845490545, 1e-9, "signrank both, approximate, n = 25");
      expect_near(hyp::signrank(vec(t1), 0.0, o).pvalue, 0.0021658340010251406, 1e-9, "signrank both, approximate, ties");
      expect_near(hyp::signrank(vec(z), 0.0, o).pvalue, 0.13941397332153205, 1e-9, "signrank both, approximate, zero differences");
    }

    // Finite-sample Jarque-Bera p-values (Monte Carlo reference with 10^5 draws, standard error about 0.002).
    {
      const auto r = hyp::jbtest(vec(a));
      expect_near(r.stat, 1.1688484001702517, 1e-9, "jbtest statistic, n = 25");
      expect_near(r.pvalue, 0.372, 0.012, "jbtest p-value, n = 25 (the chi-squared limit would give 0.557)");
      expect_near(r.critical, 4.156, 0.15, "jbtest 5% critical value, n = 25");
      expect_near(hyp::jbtest(vec(t1)).pvalue, 0.635, 0.012, "jbtest p-value, n = 12");
    }
    std::cout << "inference_reference_values_test: all checks passed\n";
  } catch (const std::exception& e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
