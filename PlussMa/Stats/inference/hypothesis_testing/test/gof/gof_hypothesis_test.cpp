#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
namespace hyp = pluss::subsp::stats::inference::hypothesis;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Series make_vec(std::vector<double> values) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, cfg);
}

Series make_mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  return pluss::make_series(values, {rows, cols}, cfg);
}

} // namespace

int main() {
  try {
    const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

    {
      const Series x = make_vec({4.8, 4.9, 5.0, 5.1, 5.2, 5.3});
      hyp::TestOptions opt;
      opt.tail = hyp::Tail::Both;
      const auto r = hyp::kstest(x, 0.0, 1.0, opt, cfg);
      expect(r.reject, "kstest reject");
      expect(r.pvalue < 0.01, "kstest pvalue");
      expect(r.effective_n == 6, "kstest effective_n");
    }

    {
      const Series x = make_vec({0.1, 0.2, 0.3, 0.4, 0.5, 0.6});
      const Series y = make_vec({5.1, 5.2, 5.3, 5.4, 5.5, 5.6});
      hyp::TestOptions opt;
      opt.tail = hyp::Tail::Both;
      const auto r = hyp::kstest2(x, y, opt, cfg);
      expect(r.reject, "kstest2 reject");
      expect(r.pvalue < 0.05, "kstest2 pvalue");
      expect(r.effective_nx == 6 && r.effective_ny == 6, "kstest2 effective_n");
    }

    {
      const Series x_heavy = make_vec({0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.3, 10.0, 20.0});
      const Series x_ref = make_vec({-1.2, -0.7, -0.3, 0.0, 0.2, 0.4, 0.7, 0.9, 1.1});
      const auto r_heavy = hyp::jbtest(x_heavy, 0.05, cfg);
      const auto r_ref = hyp::jbtest(x_ref, 0.05, cfg);
      expect(r_heavy.stat > r_ref.stat, "jbtest heavy-sample larger stat");
      expect(r_heavy.pvalue < r_ref.pvalue, "jbtest heavy-sample smaller pvalue");
      expect(r_heavy.effective_n == 9, "jbtest effective_n");
      expect(std::isfinite(r_heavy.skewness) && std::isfinite(r_heavy.kurtosis), "jbtest moments finite");
    }

    {
      const Series obs = make_vec({18.0, 21.0, 16.0, 25.0});
      const Series exp = make_vec({20.0, 20.0, 20.0, 20.0});
      hyp::ChiGofOptions opt;
      opt.alpha = 0.05;
      const auto r = hyp::chisq_gof(obs, exp, opt, cfg);
      expect(!r.reject, "chisq_gof non-reject");
      expect(r.bins == 4, "chisq_gof bins");
      expect(r.expected.defined() && r.expected.numel() == 4, "chisq_gof expected shape");
    }

    {
      const Series table = make_mat(
          {
              90.0, 60.0, 104.0,
              30.0, 50.0, 51.0,
          },
          2,
          3);
      const auto r = hyp::chisq_indep(table, 0.05, cfg);
      expect(r.reject, "chisq_indep reject");
      expect(r.rows == 2 && r.cols == 3, "chisq_indep shape");
      expect(r.expected.defined() && r.expected.numel() == 6, "chisq_indep expected shape");
    }

    {
      // exact Kolmogorov-Smirnov p-values; reference values from an independent implementation
      const auto v = [](std::vector<double> d) { return pluss::make_series(d, {static_cast<std::int64_t>(d.size())}).to(ScalarType::Float64); };
      const auto a = v({5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1});
      const auto b = v({4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3});
      expect(std::abs(hyp::kstest(a, 5.5, 0.5).pvalue - 0.7316200977) < 1e-6, "one-sample KS exact p-value");
      expect(std::abs(hyp::kstest2(a, b).pvalue - 0.1423282) < 1e-6, "two-sample KS exact p-value");
      hyp::TestOptions approx;
      approx.method = hyp::PValueMethod::Approximate;
      expect(std::abs(hyp::kstest(a, 5.5, 0.5, approx).pvalue - 0.7316200977) > 1e-3, "approximate and exact KS p-values differ at n = 8");
    }

    std::cout << "gof_hypothesis_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "gof_hypothesis_test failed: " << e.what() << '\n';
    return 1;
  }
}
