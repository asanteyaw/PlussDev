<!-- generated from a_path_with_jumps.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A path with jumps

[Back to Jumps and the Marcus Integral](../stochastic_03_jumps_and_marcus.md)


`simulate_jump_diffusion_path` builds `X = drift · t + volatility · W + jumps` on a grid, with Gaussian jump sizes, and returns the mask of the steps that contain a jump. At most one jump is simulated per step, and the call refuses a grid on which that would not be a good approximation.

```cpp
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
```

Output

```text
steps with a jump: 5.0000 of 4000
quadratic variation of the path: 0.7548 (a continuous path with volatility 0.2 would give 0.04)
ratio to elapsed time: 0.7548
warning: the path's realized quadratic variation is 0.754848 times the elapsed time; a standard Wiener path gives about 1, so this is not a standard Brownian path (another volatility, a smooth path, or jumps) and Ito and Stratonovich values are not the standard ones
```
Five of the 4000 steps contain a jump, and they dominate the quadratic variation: 0.7548, against 0.04 for a continuous path with volatility 0.2. The diagnostics of chapter 1 notice this and warn. A path like this is a legitimate integrator; the warning only says that Ito and Stratonovich values are not the standard ones.
