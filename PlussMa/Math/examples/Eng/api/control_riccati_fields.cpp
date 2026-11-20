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
  const lti::StateSpace system = lti::state(A2(), B2(), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({1.0}, 1, 1);
  const ctl::RiccatiResult sol = ctl::lqr(system, q, r);
  out.writeln("value  (P)  shape", sol.value.size(0), "x", sol.value.size(1), "=", sol.value.flatten().items<double>());
  out.writeln("gain   (K)  shape", sol.gain.size(0), "x", sol.gain.size(1), "=", sol.gain.flatten().items<double>());
  out.writeln("closed (A - B K) shape", sol.closed_loop.size(0), "x", sol.closed_loop.size(1), "=", sol.closed_loop.flatten().items<double>());
}
