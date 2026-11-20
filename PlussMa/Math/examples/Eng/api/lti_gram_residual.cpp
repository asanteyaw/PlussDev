#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
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

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const lti::StateSpace s = oscillator();
  const pluss::Series w = lti::gram(s, lti::Gram::Reach);
  const pluss::Series residual = s.a().matmul(w) + w.matmul(s.a().transpose(0, 1)) + s.b().matmul(s.b().transpose(0, 1));
  out.writeln("reachability Gramian =", w.flatten().items<double>());
  out.writeln("largest entry of A W + W A' + B B' =", residual.abs().max().item<double>());
  const pluss::Series o = lti::gram(s, lti::Gram::Observe);
  const pluss::Series residual_o = s.a().transpose(0, 1).matmul(o) + o.matmul(s.a()) + s.c().transpose(0, 1).matmul(s.c());
  out.writeln("largest entry of A' W + W A + C' C for the observability Gramian =", residual_o.abs().max().item<double>());
}
