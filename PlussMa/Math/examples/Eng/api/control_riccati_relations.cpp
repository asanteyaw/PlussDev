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
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({4.0}, 1, 1);
  const ctl::RiccatiResult sol = ctl::care(A2(), B2(), q, r);
  const pluss::Series k = B2().transpose(0, 1).matmul(sol.value).div(4.0);
  out.writeln("K - R^-1 B' P, largest entry =", max_abs(sol.gain - k));
  out.writeln("closed loop - (A - B K), largest entry =", max_abs(sol.closed_loop - (A2() - B2().matmul(sol.gain))));
}
