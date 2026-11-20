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
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});
  const pluss::Series b = pluss::make_series({0.5, 0.5});
  const pluss::Series a = pluss::make_series({1.0, -0.3});
  const auto once = sig::filter(x, b, a).values.items<double>();
  const sig::FilterRun first = sig::filter(x.slice(0, 0, 3), b, a);
  const sig::FilterRun second = sig::filter(x.slice(0, 3, 8), b, a, first.state);
  const auto joined = first.values.unite({second.values}, 0).items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < once.size(); ++i) worst = std::max(worst, std::abs(once[i] - joined[i]));
  out.writeln("one pass       =", once);
  out.writeln("two blocks     =", joined);
  out.writeln("largest difference between the two =", worst);
}
