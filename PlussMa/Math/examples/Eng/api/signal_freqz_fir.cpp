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
  const pluss::Series taps = pluss::make_series({0.5, 0.5});
  const double pi = std::acos(-1.0);
  const pluss::Series omega = pluss::make_series({0.0, pi / 2.0, pi});
  const auto h = sig::freqz(taps, omega).items<std::complex<double>>();
  for (std::size_t i = 0; i < h.size(); ++i) {
    const double omega_i = i == 0 ? 0.0 : (i == 1 ? pi / 2.0 : pi);
    out.writeln("omega", omega_i, ": |H| =", std::abs(h[i]), " expected |cos(omega / 2)| =", std::abs(std::cos(omega_i / 2.0)));
  }
}
