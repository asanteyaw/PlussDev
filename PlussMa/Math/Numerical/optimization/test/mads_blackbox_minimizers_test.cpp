#include <pluss/tils/aux/scalar_cast_compat.h>
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

Series sphere(const Series& x) {
  return x.square().sum();
}

Series bounded_shifted_sphere(const Series& x) {
  return x.sub(3.0).square().sum();
}

} // namespace

int main() {
  int failures = 0;

  const std::int64_t dim = 8;
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);
  const Series x0 = pluss::series::full({dim}, 2.0, cfg);
  const Series lb = pluss::series::full({dim}, -1.0, cfg);
  const Series ub = pluss::series::full({dim}, 1.0, cfg);

  MadsBlackboxConfig ortho_cfg;
  ortho_cfg.max_iterations = 900;
  ortho_cfg.max_evaluations = 3500;
  ortho_cfg.check_every = 20;
  ortho_cfg.search_points = 6;
  ortho_cfg.mesh_tolerance = 1e-7;
  ortho_cfg.poll_scheme = MadsPollScheme::Ortho2N;
  ortho_cfg.rng_seed = 7071ULL;

  const auto ortho_out = orthomads_minimizer(sphere, x0, ortho_cfg);
  std::cerr << "orthomads f=" << ortho_out.minimum << '\n';
  expect(std::isfinite(ortho_out.minimum), "orthomads produced non-finite objective", &failures);
  expect(ortho_out.minimum < 1e-5, "orthomads failed sphere accuracy target", &failures);

  MadsBlackboxConfig lt_cfg = ortho_cfg;
  lt_cfg.poll_scheme = MadsPollScheme::LtMadsRandom2N;
  lt_cfg.rng_seed = 7072ULL;
  const auto lt_out = ltmads_minimizer(sphere, x0, lt_cfg);
  std::cerr << "ltmads f=" << lt_out.minimum << '\n';
  expect(std::isfinite(lt_out.minimum), "ltmads produced non-finite objective", &failures);
  expect(lt_out.minimum < 5e-4, "ltmads failed sphere accuracy target", &failures);

  MadsBlackboxConfig rep_cfg = ortho_cfg;
  rep_cfg.rng_seed = 9001ULL;
  const auto rep_a = mesh_adaptive_direct_search_minimizer(sphere, x0, rep_cfg);
  const auto rep_b = mesh_adaptive_direct_search_minimizer(sphere, x0, rep_cfg);
  expect(std::abs(rep_a.minimum - rep_b.minimum) < 1e-12,
         "MADS reproducibility objective mismatch", &failures);
  expect(as_f64(rep_a.minimizer.sub(rep_b.minimizer).abs().amax(0, false)) < 1e-10,
         "MADS reproducibility minimizer mismatch", &failures);

  MadsBlackboxConfig bound_cfg = ortho_cfg;
  bound_cfg.lower_bounds = lb;
  bound_cfg.upper_bounds = ub;
  bound_cfg.rng_seed = 9002ULL;
  const auto bound_out =
      mesh_adaptive_direct_search_minimizer(bounded_shifted_sphere, x0, bound_cfg);
  const Series xb = bound_out.minimizer.to(pCPU, ScalarType::Float64);
  const double xb_min = as_f64(xb.amin(0, false));
  const double xb_max = as_f64(xb.amax(0, false));
  expect(xb_min >= -1.000001 && xb_max <= 1.000001,
         "bounded MADS returned value outside bounds", &failures);
  expect(as_f64(xb.mean()) > 0.85, "bounded MADS did not approach constrained optimum on upper face", &failures);

  {
    // check_every = 0 switches the improvement-window test off; the run must still converge on the frame size alone.
    MadsBlackboxConfig frame_only;
    frame_only.check_every = 0;
    frame_only.max_iterations = 4000;
    frame_only.max_evaluations = 40000;
    const Series start = pluss::make_series(std::vector<double>{3.0, -2.0}, {2}, cfg);
    const auto out = orthomads_minimizer(
        [](const Series& x) { return x.sub(1.0).square().sum().add(1.0); }, start, frame_only);
    expect(out.status == BlackboxSolverStatus::Converged && std::abs(out.minimum - 1.0) < 1e-6,
           "MADS with the window test off converges on the frame size", &failures);
  }

  if (failures != 0) {
    std::cerr << "mads_blackbox_minimizers_test: " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "mads_blackbox_minimizers_test: all checks passed\n";
  return 0;
}
