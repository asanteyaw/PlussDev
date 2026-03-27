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

Series make_vec(std::vector<double> values) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, cfg);
}

} // namespace

int main() {
  try {
    const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

    {
      const Series x = make_vec({5.0, 6.0, 7.0, 8.0, 9.0});
      const Series y = make_vec({0.0, 1.0, 2.0, 3.0, 4.0});
      hyp::TestOptions opt;
      opt.tail = hyp::Tail::Right;
      const auto r = hyp::ranksum(x, y, opt, cfg);
      expect(r.reject, "ranksum reject");
      expect(r.pvalue < 0.01, "ranksum pvalue");
      expect(r.effective_nx == 5 && r.effective_ny == 5, "ranksum effective_n");
    }

    {
      const Series x = make_vec({1.0, 2.0, 3.0, 4.0, 5.0, 7.0, 8.0, 9.0});
      hyp::TestOptions opt;
      opt.tail = hyp::Tail::Right;
      const auto r = hyp::signrank(x, 0.0, opt, cfg);
      expect(r.reject, "signrank one-sample reject");
      expect(r.pvalue < 0.01, "signrank one-sample pvalue");
      expect(r.effective_n == 8, "signrank one-sample n");
    }

    {
      const double nan = std::numeric_limits<double>::quiet_NaN();
      const Series x = make_vec({10.0, 12.0, 11.0, 13.0, 15.0, 14.0, 16.0, nan});
      const Series y = make_vec({8.0, 9.0, 10.0, 10.0, 13.0, 12.0, 14.0, 15.0});
      hyp::TestOptions opt;
      opt.tail = hyp::Tail::Right;
      const auto r = hyp::signrank(x, y, opt, cfg);
      expect(r.reject, "signrank paired reject");
      expect(r.pvalue < 0.05, "signrank paired pvalue");
      expect(r.effective_n == 7, "signrank paired drops NaN pairs");
    }

    {
      const Series x = make_vec({1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9});
      hyp::TestOptions opt;
      opt.tail = hyp::Tail::Right;
      const auto r = hyp::signtest(x, 0.0, opt, cfg);
      expect(r.reject, "signtest one-sample reject");
      expect(r.pvalue < 0.01, "signtest one-sample pvalue");
      expect(r.positives == 10 && r.negatives == 0, "signtest one-sample counts");
    }

    {
      const Series x = make_vec({3.0, 4.0, 5.0, 6.0, 7.0, 8.0});
      const Series y = make_vec({2.0, 3.0, 4.0, 4.0, 6.0, 6.0});
      hyp::TestOptions opt;
      opt.tail = hyp::Tail::Right;
      const auto r = hyp::signtest(x, y, opt, cfg);
      expect(r.reject, "signtest paired reject");
      expect(r.pvalue < 0.05, "signtest paired pvalue");
      expect(r.effective_n == 6, "signtest paired n");
    }

    std::cout << "nonparametric_hypothesis_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "nonparametric_hypothesis_test failed: " << e.what() << '\n';
    return 1;
  }
}
