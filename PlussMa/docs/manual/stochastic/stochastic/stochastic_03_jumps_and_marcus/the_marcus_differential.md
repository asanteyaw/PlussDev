<!-- generated from the_marcus_differential.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The Marcus differential

[Back to Jumps and the Marcus Integral](../stochastic_03_jumps_and_marcus.md)


`x.calc.diff.marcus(g, mu, sigma, h, jumps)` returns the differential of `g(X)` for a Stratonovich diffusion plus jumps that follow the flow of `h`. For `g = log` and `h = x` the jump moves `log X` by exactly the atom.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series x = pluss::make_series(std::vector<double>{1.5}, {1}).to(f64);
  const pluss::Series mu = x.mul(0.1), sigma = x.mul(0.3);
  pluss::Series::JumpMeasure jumps;
  jumps.sizes = {pluss::make_series(std::vector<double>{0.4, -0.7}, {2}).to(f64)};
  jumps.intensity = pluss::make_series(std::vector<double>{2.0, 1.0}, {2}).to(f64);

  const auto d = x.calc.diff.marcus([](const pluss::Series& s) { return s.log(); }, mu, sigma,
                                    [](const pluss::Series& s) { return s; }, jumps);
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "continuous drift  " << d.drift.squeeze().item<double>() << "  (mu / x = " << 0.1 << ")\n";
  std::cout << "diffusion         " << d.diffusion.squeeze().item<double>() << "  (sigma / x = " << 0.3 << ")\n";
  const auto js = d.jump_sizes.host<double>();
  std::cout << "jump sizes of log X: " << js[0] << ", " << js[1] << "  (the atoms 0.4 and -0.7)\n";
  return 0;
}
```

Output

```text
continuous drift  0.100000  (mu / x = 0.100000)
diffusion         0.300000  (sigma / x = 0.300000)
jump sizes of log X: 0.400000, -0.700000  (the atoms 0.4 and -0.7)
```
The continuous part is the Stratonovich one: drift `μ/x = 0.1` and diffusion `σ/x = 0.3`, with no second-order term. The jump sizes of `log X` are 0.4 and −0.7, the atoms themselves.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="what_a_jump_does_to_the_state.html">
    <span class="label">Previous</span>
    <span class="title">What a jump does to the state</span>
  </a>
</div>
<!-- apiref:nav:end -->
