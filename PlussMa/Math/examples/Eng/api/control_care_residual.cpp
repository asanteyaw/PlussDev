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
  const pluss::Series r = mat({1.0}, 1, 1);
  const ctl::RiccatiResult sol = ctl::care(A2(), B2(), q, r);
  const pluss::Series &p = sol.value;
  const pluss::Series residual = A2().transpose(0, 1).matmul(p) + p.matmul(A2()) - p.matmul(B2()).matmul(B2().transpose(0, 1)).matmul(p) + q;
  out.writeln("P =", p.flatten().items<double>());
  out.writeln("largest entry of A'P + PA - P B R^-1 B' P + Q =", max_abs(residual));
  out.writeln("P is symmetric: largest |P - P'| =", max_abs(p - p.transpose(0, 1)));
  const auto k = sol.gain.flatten().items<double>();
  out.writeln("K =", k, " closed-loop trace =", sol.closed_loop.flatten().items<double>()[0] + sol.closed_loop.flatten().items<double>()[3], "(negative: both eigenvalues in the left half-plane for this system)");
}
