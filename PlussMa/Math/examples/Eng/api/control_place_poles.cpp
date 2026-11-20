#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // x'' = u: place the closed-loop poles at -2 and -3, so det(sI - A + BK) = s^2 + 5 s + 6.
  const pluss::Series poles = pluss::make_series({-2.0, -3.0});
  const ctl::PlaceResult r = ctl::place(A2(), B2(), poles);
  out.writeln("gain K =", r.gain.flatten().items<double>(), " expected [6, 5]");
  const auto m = r.closed_loop.flatten().items<double>();
  out.writeln("closed loop trace =", m[0] + m[3], " (sum of the poles -5)  determinant =", m[0] * m[3] - m[1] * m[2], " (product of the poles 6)");
}
