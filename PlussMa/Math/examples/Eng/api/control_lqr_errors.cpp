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
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64);
  refused(out, "R that is not positive definite", [&] { ctl::lqr(system, q, mat({0.0}, 1, 1)); });
  refused(out, "Q of the wrong size", [&] { ctl::lqr(system, mat({1.0}, 1, 1), mat({1.0}, 1, 1)); });
  const lti::StateSpace stuck = lti::state(mat({1.0, 0.0, 0.0, 2.0}, 2, 2), mat({1.0, 0.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  refused(out, "a system that cannot be stabilised", [&] { ctl::lqr(stuck, q, mat({1.0}, 1, 1)); });
}
