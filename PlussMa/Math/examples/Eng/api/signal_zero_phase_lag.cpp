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
  const pluss::Series pulse = pluss::make_series({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  const auto peak_of = [](const pluss::Series &s) {
    const auto v = s.items<double>();
    return std::max_element(v.begin(), v.end()) - v.begin();
  };
  const pluss::Series causal = sig::filter(pulse, b, a).values;
  const pluss::Series centred = sig::zero_phase(pulse, b, a, {.dim = 0, .edge = 2});
  out.writeln("the pulse peaks at sample", peak_of(pulse));
  out.writeln("the causal filter moves the peak to sample", peak_of(causal), "(it lags)");
  out.writeln("zero_phase keeps the peak at sample", peak_of(centred));
  out.writeln("zero-phase output =", centred.items<double>());
}
