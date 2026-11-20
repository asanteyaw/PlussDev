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
  // Observability of (A, C) is controllability of (A', C'): the observability matrix is the transpose of the controllability matrix of the dual.
  const lti::StateSpace s = oscillator();
  const lti::StateSpace dual = lti::state(s.a().transpose(0, 1), s.c().transpose(0, 1), s.b().transpose(0, 1), s.d().transpose(0, 1));
  out.writeln("obsv(system)       =", lti::obsv(s).flatten().items<double>());
  out.writeln("ctrb(dual)^T       =", lti::ctrb(dual).transpose(0, 1).flatten().items<double>());
}
