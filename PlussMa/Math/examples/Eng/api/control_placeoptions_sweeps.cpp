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
  // Four states driven through two inputs: with more than one input the eigenvector basis can be refined, sweep by sweep.
  const pluss::Series a = mat({0, 1, 0, 0, 0, 0, 0.2, 0, 0, 0, 0, 1, 0.1, 0, 0, 0}, 4, 4);
  const pluss::Series b = mat({0, 0, 1, 0, 0, 0, 0, 1}, 4, 2);
  const pluss::Series poles = pluss::make_series({-1.0, -1.5, -2.0, -2.5});
  for (const std::int64_t sweeps : {0, 1, 3, 5}) {
    const ctl::PlaceResult r = ctl::place(a, b, poles, {.sweeps = sweeps});
    out.writeln("sweeps allowed", sweeps, ": sweeps used", r.sweeps, " condition of the basis", r.condition.max().item<double>());
  }
}
