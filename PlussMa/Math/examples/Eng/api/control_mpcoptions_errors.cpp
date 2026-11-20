#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace mpc = pluss::subsp::eng::control::mpc;
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

// A discrete integrator x[k+1] = x[k] + u[k] to be driven from 0 to the reference 1.
static lti::StateSpace plant() { return lti::state(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
static mpc::Weights weights(double r = 0.1, double terminal = 2.0) { return {mat({1.0}, 1, 1), mat({r}, 1, 1), mat({terminal}, 1, 1)}; }
static mpc::Bounds generous() { return {pluss::make_series({-10.0}), pluss::make_series({10.0})}; }
static mpc::Options with_horizon(std::int64_t n) { mpc::Options o; o.horizon = n; o.qp.abs_tol = 1e-8; o.qp.rel_tol = 1e-8; return o; }
static mpc::Result run(double r = 0.1, const mpc::Bounds &b = generous(), std::int64_t n = 4, double terminal = 2.0) { return mpc::solve(plant(), pluss::make_series({0.0}), pluss::make_series({1.0}), weights(r, terminal), b, with_horizon(n)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  refused(out, "a horizon of zero", [] { mpc::Options o; o.horizon = 0; mpc::build(plant(), weights(), generous(), o); });
  refused(out, "a negative horizon", [] { mpc::Options o; o.horizon = -3; mpc::build(plant(), weights(), generous(), o); });
}
