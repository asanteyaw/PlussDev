#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
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

bool approx(double a, double b, double tol = 1e-9) {
  return std::abs(a - b) <= tol;
}

Series make_vec(std::vector<double> values) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, cfg);
}

} // namespace

int main() {
  try {
    const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

    {
      const Series x = make_vec({2.0, 3.0, 4.0, 5.0, 6.0});
      hyp::TestOptions opt;
      opt.alpha = 0.05;
      opt.tail = hyp::Tail::Both;
      const auto r = hyp::ttest(x, 4.0, opt, cfg);
      expect(std::abs(r.stat) < 1e-12, "ttest zero-shift stat");
      expect(r.pvalue > 0.99, "ttest zero-shift pvalue");
      expect(!r.reject, "ttest zero-shift reject");
      expect(r.effective_n == 5, "ttest effective_n");
      expect(r.ci.defined() && r.ci.numel() == 2, "ttest ci shape");
    }

    {
      const Series x = make_vec({4.0, 5.0, 6.0, 7.0, 8.0});
      hyp::TestOptions opt;
      opt.alpha = 0.05;
      opt.tail = hyp::Tail::Right;
      const auto r = hyp::ttest(x, 3.0, opt, cfg);
      expect(r.reject, "ttest right-tail reject");
      expect(r.pvalue < 0.02, "ttest right-tail pvalue");
      expect(std::isfinite(r.critical), "ttest right-tail critical finite");
    }

    {
      const double nan = std::numeric_limits<double>::quiet_NaN();
      const Series x = make_vec({5.0, 6.0, nan, 10.0});
      const Series y = make_vec({1.0, 2.0, 3.0, 4.0});
      hyp::TestOptions opt;
      opt.tail = hyp::Tail::Right;
      const auto r = hyp::ttest_paired(x, y, opt, cfg);
      expect(r.reject, "ttest_paired reject");
      expect(r.effective_n == 3, "ttest_paired drops NaN pairs");
    }

    {
      const Series x = make_vec({10.2, 9.7, 11.1, 10.5, 9.9, 10.8});
      const Series y = make_vec({1.2, -0.1, 0.4, 1.1, 0.6, -0.3});
      hyp::TTest2Options opt;
      opt.alpha = 0.05;
      opt.tail = hyp::Tail::Both;
      opt.equal_variance = true;
      const auto r = hyp::ttest2(x, y, opt, cfg);
      expect(r.reject, "ttest2 equal-var reject");
      expect(r.pvalue < 1e-4, "ttest2 equal-var pvalue");
      expect(r.equal_variance, "ttest2 equal-var flag");
      expect(r.ci.defined() && r.ci.numel() == 2, "ttest2 equal-var ci shape");
    }

    {
      const Series x = make_vec({7.0, 7.5, 8.0, 8.3, 8.5, 8.7});
      const Series y = make_vec({2.0, 2.1, 2.0, 1.9, 2.2, 2.1});
      hyp::TTest2Options opt;
      opt.alpha = 0.05;
      opt.tail = hyp::Tail::Right;
      opt.equal_variance = false;
      const auto r = hyp::ttest2(x, y, opt, cfg);
      expect(r.reject, "ttest2 welch reject");
      expect(r.pvalue < 1e-4, "ttest2 welch pvalue");
      expect(!r.equal_variance, "ttest2 welch flag");
      expect(r.dof > 0.0, "ttest2 welch dof positive");
    }

    {
      const Series x = make_vec({1.5, 1.7, 1.6, 1.8, 1.4, 1.9});
      hyp::TestOptions opt;
      opt.tail = hyp::Tail::Right;
      const auto r = hyp::ztest(x, 1.0, 0.2, opt, cfg);
      expect(r.reject, "ztest reject");
      expect(r.pvalue < 1e-6, "ztest pvalue");
      expect(approx(r.sigma, 0.2), "ztest sigma carried");
    }

    {
      const Series x = make_vec({0.5, 1.0, 1.5, 2.0, 2.2, 1.8, 0.9});
      hyp::TestOptions opt;
      const auto r = hyp::vartest(x, 1.0, opt, cfg);
      expect(r.pvalue >= 0.0 && r.pvalue <= 1.0, "vartest pvalue bounds");
      expect(r.ci.defined() && r.ci.numel() == 2, "vartest ci shape");
      expect(r.effective_n == 7, "vartest effective_n");
    }

    {
      const Series x = make_vec({-4.0, -1.0, 2.0, 5.0, 8.0, 11.0});
      const Series y = make_vec({0.9, 1.0, 1.1, 0.95, 1.05, 1.0});
      hyp::TestOptions opt;
      opt.tail = hyp::Tail::Right;
      const auto r = hyp::vartest2(x, y, opt, cfg);
      expect(r.reject, "vartest2 right-tail reject");
      expect(r.pvalue < 1e-4, "vartest2 right-tail pvalue");
      expect(r.dof1 == 5.0 && r.dof2 == 5.0, "vartest2 dof");
      expect(r.ci.defined() && r.ci.numel() == 2, "vartest2 ci shape");
    }

    {
      const Series y = make_vec({1.0, 1.2, 0.8, 3.1, 2.9, 3.0, 6.0, 6.2, 5.9});
      const Series g = make_vec({0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0});
      hyp::Anova1WOptions opt;
      opt.alpha = 0.05;
      const auto r = hyp::anova_1w(y, g, opt, cfg);
      expect(r.reject, "anova_1w reject");
      expect(r.groups == 3, "anova_1w group count");
      expect(r.group_means.defined() && r.group_means.numel() == 3, "anova_1w means shape");
      expect(r.group_counts.defined() && r.group_counts.numel() == 3, "anova_1w counts shape");
    }

    {
      const Series x = make_vec({-4.0, -1.0, 2.0, 5.0, 8.0, 11.0});
      const Series y = make_vec({0.9, 1.0, 1.1, 0.95, 1.05, 1.0});
      hyp::TestOptions opt;
      opt.tail = hyp::Tail::Right;
      const auto r_f = hyp::f_ratio(x, y, opt, cfg);
      const auto r_v = hyp::vartest2(x, y, opt, cfg);
      expect(approx(r_f.stat, r_v.stat, 1e-12), "f_ratio stat parity");
      expect(approx(r_f.pvalue, r_v.pvalue, 1e-12), "f_ratio pvalue parity");
    }

    std::cout << "parametric_hypothesis_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "parametric_hypothesis_test failed: " << e.what() << '\n';
    return 1;
  }
}
