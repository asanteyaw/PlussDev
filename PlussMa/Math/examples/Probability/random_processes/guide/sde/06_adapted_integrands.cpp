// An Ito sum is only an Ito integral if the integrand at step i was computed from the path up to sample i. An
// integrand built from the whole sampled path can break this without any error message: a centered moving average
// reads one sample ahead, a terminal value reads all of them. The sums still run and return numbers, but they are
// not Ito integrals. PlussMa's ito_integral and strat_integral take the integrand as a function of the path and
// probe it first: they redraw the later part of the path and require the integrand at the earlier samples to stay
// the same.
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const Series like = pluss::make_series(0.0).to(f64).reshape({1});
  const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, 200, /*seed=*/9);

  // adapted: the running maximum of the path up to each sample
  auto running_max = [](const Series& w) {
    Series best = w.select(0, 0);
    Series out = best.unsqueeze(0);
    for (std::int64_t i = 1; i < w.size(0); ++i) {
      best = best.maximum(w.select(0, i));
      out = out.unite({best.unsqueeze(0)}, 0);
    }
    return out;
  };
  // anticipating: average of the previous, current and next sample
  auto centered_average = [](const Series& w) {
    const std::int64_t n = w.size(0);
    const Series previous = w.slice(0, 0, 1, 1).unite({w.slice(0, 0, n - 1, 1)}, 0);
    const Series next = w.slice(0, 1, n, 1).unite({w.slice(0, n - 1, n, 1)}, 0);
    return previous.add(w).add(next).div(3.0);
  };
  // anticipating: the value the path ends at
  auto terminal_value = [](const Series& w) {
    return w.select(0, w.size(0) - 1).unsqueeze(0).expand({w.size(0), w.size(1)});
  };

  struct Case { const char* label; sde::SampledIntegrandFn make; };
  const std::vector<Case> cases = {{"running maximum", running_max},
                                   {"centered moving average", centered_average},
                                   {"terminal value", terminal_value}};
  for (const auto& c : cases) {
    try {
      const auto r = sde::ito_integral(c.make, path);
      std::cout << c.label << ": accepted, Ito integral = " << r.value.item<double>() << "\n";
    } catch (const std::exception& e) {
      std::string message = e.what();
      message = message.substr(0, message.find('\n'));
      std::cout << c.label << ": rejected\n    " << message << "\n";
    }
  }
  return 0;
}
