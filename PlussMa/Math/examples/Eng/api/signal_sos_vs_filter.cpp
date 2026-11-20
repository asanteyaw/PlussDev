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
  // Two sections b0 b1 b2 a0 a1 a2: a one-pole lowpass pair.
  const pluss::Series sections = pluss::make_series({0.5, 0.5, 0.0, 1.0, -0.2, 0.0, 0.5, 0.5, 0.0, 1.0, -0.4, 0.0}).reshape({2, 6});
  const pluss::Series x = pluss::make_series({1.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  const auto cascade = sig::sos(x, sections).values.items<double>();
  // The same filter as one polynomial: the products of the numerators and of the denominators.
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0, -0.6, 0.08});
  const auto direct = sig::filter(x, b, a).values.items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < cascade.size(); ++i) worst = std::max(worst, std::abs(cascade[i] - direct[i]));
  out.writeln("cascade impulse response =", cascade);
  out.writeln("single-polynomial response =", direct);
  out.writeln("largest difference =", worst);
}
