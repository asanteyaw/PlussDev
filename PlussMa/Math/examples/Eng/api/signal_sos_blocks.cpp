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
  const pluss::Series sections = pluss::make_series({0.5, 0.5, 0.0, 1.0, -0.2, 0.0, 0.5, 0.5, 0.0, 1.0, -0.4, 0.0}).reshape({2, 6});
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  const auto once = sig::sos(x, sections).values.items<double>();
  const sig::FilterRun first = sig::sos(x.slice(0, 0, 2), sections);
  const sig::FilterRun second = sig::sos(x.slice(0, 2, 6), sections, first.state);
  const auto joined = first.values.unite({second.values}, 0).items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < once.size(); ++i) worst = std::max(worst, std::abs(once[i] - joined[i]));
  out.writeln("state shape: sections", first.state.size(0), "by", first.state.size(1));
  out.writeln("largest difference between one pass and two blocks =", worst);
}
