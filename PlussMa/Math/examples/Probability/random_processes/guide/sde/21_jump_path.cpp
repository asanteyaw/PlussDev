// A path that jumps. simulate_jump_diffusion_path builds X = drift t + volatility W + compound-Poisson jumps on a
// grid and reports the steps that contain a jump. The path diagnostics notice that the quadratic variation is not
// the elapsed time: jumps add their squares to it, so a path like this is a legitimate integrator but not a
// standard Wiener path.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series like = pluss::make_series(0.0).to(f64).reshape({1});
  const auto jp = sde::simulate_jump_diffusion_path(like, 0.0, 1.0, 4000, /*seed=*/11, /*drift=*/0.0, /*volatility=*/0.2,
                                                    /*jump_rate=*/6.0, /*jump_mean=*/0.0, /*jump_stddev=*/0.5);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "steps with a jump: " << jp.jump_mask.sum().item<double>() << " of " << jp.path.steps << "\n";
  const auto r = sde::ito_integral([](double, const pluss::Series& w) { return w; }, jp.path);
  std::cout << "quadratic variation of the path: " << r.realized_quadratic_variation.item<double>()
            << " (a continuous path with volatility 0.2 would give 0.04)\n";
  std::cout << "ratio to elapsed time: " << r.quadratic_variation_ratio << "\n";
  for (const auto& w : r.warnings) std::cout << "warning: " << w << "\n";
  return 0;
}
