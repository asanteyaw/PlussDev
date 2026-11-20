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
  const pluss::Series sections = pluss::make_series({0.25, 0.5, 0.25, 1.0, 0.0, 0.0}).reshape({1, 6});
  const pluss::Series pulse = pluss::make_series({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  const auto by_sections = sig::zero_phase(pulse, sections, {.dim = 0, .edge = 2}).items<double>();
  const auto by_polynomial = sig::zero_phase(pulse, b, a, {.dim = 0, .edge = 2}).items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < by_sections.size(); ++i) worst = std::max(worst, std::abs(by_sections[i] - by_polynomial[i]));
  out.writeln("sections overload against polynomial overload, largest difference =", worst);
}
