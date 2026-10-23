// The mesh. The mesh size is the step of the poll: it grows by `mesh_expansion` after a success and shrinks by
// `mesh_contraction` after a failed poll; the run converges when it falls below `mesh_tolerance`. A start that is too small
// takes many expansions; one that is too large takes many contractions. `mesh_scaling` also rescales the step per
// variable when the variables have very different ranges (Off, RangeLog, NupsDynamic). The problem here has one variable
// of order 1 and one of order 1000, with bounds so that the ranges are known.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

void run(const char* label, dfo::PSConfig c) {
  c.lower_bounds = vec({-10.0, -5000.0});
  c.upper_bounds = vec({10.0, 5000.0});
  auto f = [](const pluss::Series& x) { return (x.select(0, 0) - 3.0).square() + ((x.select(0, 1) - 2000.0) / 1000.0).square(); };
  const auto o = dfo::pattern_search_minimizer(f, vec({0.0, 0.0}), c);
  std::cout << std::left << std::setw(34) << label << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2)
            << std::setw(11) << o.minimum << std::defaultfloat << "evals " << o.diagnostics.evaluations << "\n";
}

int main() {
  dfo::PSConfig base;
  run("defaults (RangeLog scaling)", base);
  auto off = base; off.mesh_scaling = dfo::PSMeshScaling::Off;
  run("mesh_scaling Off", off);
  auto nups = base; nups.mesh_scaling = dfo::PSMeshScaling::NupsDynamic;
  run("mesh_scaling NupsDynamic", nups);
  auto small = base; small.initial_mesh_size = 0.01;
  run("initial_mesh_size 0.01", small);
  auto large = base; large.initial_mesh_size = 100.0;
  run("initial_mesh_size 100", large);
  auto slow = base; slow.mesh_contraction = 0.9; slow.mesh_expansion = 1.1;
  run("expansion 1.1, contraction 0.9", slow);
}
