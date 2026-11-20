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
  // a = 1, b = 1, q = 1, r = 1: the Riccati equation 2 p - p^2 + 1 = 0 has the stabilising root p = 1 + sqrt(2).
  const ctl::RiccatiResult r = ctl::care(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1));
  out.writeln("P =", r.value.flatten().items<double>(), " exact 1 + sqrt(2) =", 1.0 + std::sqrt(2.0));
  out.writeln("K = R^-1 B' P =", r.gain.flatten().items<double>(), " closed loop a - b k =", r.closed_loop.flatten().items<double>(), " exact -sqrt(2) =", -std::sqrt(2.0));
}
