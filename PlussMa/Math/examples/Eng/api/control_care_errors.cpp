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
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64);
  refused(out, "R that is not positive definite", [&] { ctl::care(A2(), B2(), q, mat({-1.0}, 1, 1)); });
  refused(out, "a pair that cannot be stabilised", [&] { ctl::care(mat({1.0, 0.0, 0.0, 2.0}, 2, 2), mat({1.0, 0.0}, 2, 1), q, mat({1.0}, 1, 1)); });
  refused(out, "B with the wrong number of rows", [&] { ctl::care(A2(), mat({1.0, 0.0, 0.0}, 3, 1), q, mat({1.0}, 1, 1)); });
  refused(out, "a cross weight that makes the joint cost indefinite", [&] { ctl::care(A2(), B2(), q, mat({1.0}, 1, 1), mat({5.0, 5.0}, 2, 1)); });
  refused(out, "Q of the wrong size", [&] { ctl::care(A2(), B2(), mat({1.0}, 1, 1), mat({1.0}, 1, 1)); });
}
