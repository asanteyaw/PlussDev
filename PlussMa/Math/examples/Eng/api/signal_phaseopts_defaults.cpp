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
  const sig::PhaseOpts defaults;
  out.writeln("default axis =", defaults.dim, " default edge =", defaults.edge, " default pad is Odd:", defaults.pad == sig::PadMode::Odd);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 4.0, 3.0, 5.0, 4.0, 6.0, 5.0, 7.0, 6.0, 8.0, 7.0, 9.0, 8.0, 10.0, 9.0});
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  const auto automatic = sig::zero_phase(x, b, a).items<double>();
  const auto explicit_edge = sig::zero_phase(x, b, a, {.dim = 0, .edge = 6}).items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < automatic.size(); ++i) worst = std::max(worst, std::abs(automatic[i] - explicit_edge[i]));
  out.writeln("default options against edge 6 (three times the filter order of 2), largest difference =", worst);
}
