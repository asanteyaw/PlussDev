#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series x = pluss::make_series({4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0});
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  for (const auto [label, pad] : {std::pair{"Odd     ", sig::PadMode::Odd}, {"Even    ", sig::PadMode::Even}, {"Constant", sig::PadMode::Constant}, {"None    ", sig::PadMode::None}}) {
    const auto y = sig::zero_phase(x, b, a, {.dim = 0, .edge = 3, .pad = pad}).items<double>();
    const auto v = x.items<double>();
    double worst = 0.0;
    for (std::size_t i = 0; i < v.size(); ++i) worst = std::max(worst, std::abs(y[i] - v[i]));
    out.writeln(label, "largest difference from the input ramp =", worst);
  }
}
