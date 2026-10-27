// Three entry points, two poll schemes. The poll directions can be an orthogonal basis rotated at every iteration by
// a Halton-sequence reflection (OrthoMADS, `MadsPollScheme::Ortho2N`) or a random lower-triangular basis (LT-MADS,
// `MadsPollScheme::LtMadsRandom2N`). `orthomads_minimizer` and `ltmads_minimizer` are the same engine with the scheme
// chosen for you; `mesh_adaptive_direct_search_minimizer` takes the scheme from `config.poll_scheme`. Rosenbrock and a
// non-smooth function (the sum of absolute deviations), same budget.
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <cmath>
#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
int main() {
  auto kink = [](const pluss::Series& x) { return (x - vec({1.0, -2.0, 0.5, 3.0})).abs().sum(); };
  dfo::MadsBlackboxConfig c;
  c.max_iterations = 4000;
  c.max_evaluations = 40000;
  auto lt = c;
  lt.poll_scheme = dfo::MadsPollScheme::LtMadsRandom2N;

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  out.writeln("Rosenbrock");
  const std::vector<std::pair<const char*, dfo::BlackboxSolverOutcome>> rosen = {
      {"orthomads_minimizer", dfo::orthomads_minimizer(rosenbrock, vec({-1.2, 1.0}), c)},
      {"ltmads_minimizer", dfo::ltmads_minimizer(rosenbrock, vec({-1.2, 1.0}), c)},
      {"mesh_adaptive..., LtMadsRandom2N", dfo::mesh_adaptive_direct_search_minimizer(rosenbrock, vec({-1.2, 1.0}), lt)}};
  for (const auto& [label, o] : rosen) out.writeln(" ", label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);

  out.writeln("sum |x - c|, 4 variables");
  const std::vector<std::pair<const char*, dfo::BlackboxSolverOutcome>> sums = {
      {"orthomads_minimizer", dfo::orthomads_minimizer(kink, vec({0, 0, 0, 0}), c)},
      {"ltmads_minimizer", dfo::ltmads_minimizer(kink, vec({0, 0, 0, 0}), c)}};
  for (const auto& [label, o] : sums) out.writeln(" ", label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);
}
