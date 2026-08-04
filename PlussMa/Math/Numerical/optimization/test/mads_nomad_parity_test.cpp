#include "../mads_blackbox_minimizers.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::pCPU;
using namespace pluss::subsp::math::optimization;

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

Series quartic2d(const Series& x) {
  return x.pow(4.0).sum();
}

} // namespace

int main() {
  int failures = 0;

  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);
  const Series x0 = pluss::series::full({2}, 5.0, cfg);
  const Series lb = pluss::series::full({2}, -20.0, cfg);
  const Series ub = pluss::series::full({2}, 20.0, cfg);

  MadsBlackboxConfig base_cfg;
  base_cfg.max_iterations = 400;
  base_cfg.max_evaluations = 100;
  base_cfg.check_every = 5;
  base_cfg.search_points = 0;
  base_cfg.relative_improvement_tolerance = 1e-12;
  base_cfg.mesh_tolerance = 1e-9;
  base_cfg.lower_bounds = lb;
  base_cfg.upper_bounds = ub;
  base_cfg.opportunistic = true;
  base_cfg.display = BlackboxSolverDisplay::None;

  MadsBlackboxConfig ortho_cfg = base_cfg;
  ortho_cfg.poll_scheme = MadsPollScheme::Ortho2N;
  ortho_cfg.rng_seed = 20260226ULL;
  const auto ortho_out = orthomads_minimizer(quartic2d, x0, ortho_cfg);
  std::cerr << "orthomads_quartic f=" << ortho_out.minimum
            << " evals=" << ortho_out.diagnostics.evaluations << '\n';
  expect(std::isfinite(ortho_out.minimum), "orthomads quartic produced non-finite objective", &failures);
  expect(ortho_out.minimum <= 1e-10, "orthomads quartic failed NOMAD-style accuracy target", &failures);

  MadsBlackboxConfig lt_cfg = base_cfg;
  lt_cfg.poll_scheme = MadsPollScheme::LtMadsRandom2N;
  lt_cfg.rng_seed = 20260227ULL;
  const auto lt_out = ltmads_minimizer(quartic2d, x0, lt_cfg);
  std::cerr << "ltmads_quartic f=" << lt_out.minimum
            << " evals=" << lt_out.diagnostics.evaluations << '\n';
  expect(std::isfinite(lt_out.minimum), "ltmads quartic produced non-finite objective", &failures);
  expect(lt_out.minimum <= 1e-6, "ltmads quartic failed NOMAD-style accuracy target", &failures);

  if (failures != 0) {
    std::cerr << "mads_nomad_parity_test: " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "mads_nomad_parity_test: all checks passed\n";
  return 0;
}

