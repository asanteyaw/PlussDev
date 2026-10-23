<!-- generated from the_marcus_integral.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The Marcus integral

[Back to Jumps and the Marcus Integral](../stochastic_03_jumps_and_marcus.md)


The chain rule says `∫ x² dx = x³/3`. A good integral of jump paths should satisfy it. The program integrates `x²` against the path of Program 2 by the Marcus rule, by the Stratonovich average, and by the Ito sum.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series like = pluss::make_series(0.0).to(f64).reshape({1});
  const auto jp = sde::simulate_jump_diffusion_path(like, 0.0, 1.0, 4000, 11, 0.0, 0.2, 6.0, 0.0, 0.5);
  auto square = [](double, const pluss::Series& x) { return x.mul(x); };

  const double xT = jp.path.w.select(0, jp.path.steps).item<double>();
  const double exact = xT * xT * xT / 3.0;
  const double marcus = sde::marcus_integral(square, jp.path, jp.jump_mask).value.item<double>();
  const double strat = sde::strat_integral(square, jp.path).value.item<double>();
  const double ito = sde::ito_integral(square, jp.path).value.item<double>();

  std::cout << std::setprecision(6);
  std::cout << "X_T^3 / 3 (chain rule)  " << exact << "\n";
  std::cout << "Marcus                  " << marcus << "   error " << std::scientific << std::abs(marcus - exact) << std::fixed << "\n";
  std::cout << "Stratonovich (average)  " << strat << "   error " << std::scientific << std::abs(strat - exact) << std::fixed << "\n";
  std::cout << "Ito                     " << ito << "   error " << std::scientific << std::abs(ito - exact) << std::fixed << "\n";

  // Without a mask the jump steps are found from their size: |dX| > jump_sigma * sqrt(step). A jump smaller
  // than that threshold (here 8 * sqrt(1/4000) = 0.13) is treated as Brownian, which is why the number differs.
  const double automatic = sde::marcus_integral(square, jp.path, 8.0).value.item<double>();
  std::cout << "Marcus, jumps found by size  " << automatic << "\n";
  return 0;
}
```

Output

```text
X_T^3 / 3 (chain rule)  0.00164677
Marcus                  0.00164654   error 2.381076e-07
Stratonovich (average)  0.079767   error 7.811979e-02
Ito                     -0.195062   error 1.967085e-01
Marcus, jumps found by size  0.001497
```
The chain-rule value is 0.00164677. Marcus gives 0.00164654, off by 2.4e-7, which is the trapezoid error of the small Brownian steps and shrinks with the grid. The Stratonovich average is off by 0.0781 and the Ito sum by 0.1967, because both treat a jump step as if the path were straight enough to average two ends or to read the start. The last line lets the library find the jump steps by size (a step whose increment exceeds 8 standard deviations); it gives 0.001497, a little off because jumps smaller than that threshold are treated as Brownian. When you know where the jumps are, pass the mask.
