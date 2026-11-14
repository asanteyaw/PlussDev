<!-- generated from values_that_were_not_simulated.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Values that were not simulated

[Back to Stochastic Integrals](../stochastic_01_integrals.md)


An integrand and integrator may come from data rather than from a simulated Wiener path. The sampled forms take the values directly. The program integrates `x²` against a sampled `x`, and computes the covariation of two sampled series.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const std::int64_t steps = 800;
  const double h = 1.0 / steps;
  auto make_path = [&](std::uint64_t seed) {
    const pluss::Series dw = pluss::series::randn({steps, 1}, pluss::Config{}.with_dtype(f64), seed).mul(std::sqrt(h));
    return pluss::Series::zeros({1, 1}, f64).unite({dw.cumsum(0)}, 0);
  };
  const pluss::Series x = make_path(1), y = make_path(2);

  // An integrand computed from the path: the square of the path so far.
  const pluss::Series f = x.mul(x);
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "Ito of x^2 dx:            " << f.antidiff(x).ito(0).squeeze().item<double>() << "\n";
  std::cout << "Stratonovich of x^2 dx:   " << f.antidiff(x).strat(0).squeeze().item<double>() << "   (x_T^3 / 3 = "
            << std::pow(x.select(0, steps).squeeze().item<double>(), 3) / 3.0 << " in the limit)\n";
  // The Ito form needs only the left endpoints: n - 1 samples are enough.
  std::cout << "Ito from n - 1 samples:   " << f.slice(0, 0, steps, 1).antidiff(x).ito(0).squeeze().item<double>() << "\n\n";

  const double qx = x.antidiff().quadratic_variation(0).squeeze().item<double>();
  const double qy = y.antidiff().quadratic_variation(0).squeeze().item<double>();
  const double cxy = x.antidiff(y).covariation(0).squeeze().item<double>();
  const double qsum = x.add(y).antidiff().quadratic_variation(0).squeeze().item<double>();
  std::cout << "[x] = " << qx << ", [y] = " << qy << ", [x, y] = " << cxy << "\n";
  std::cout << "[x + y] = " << qsum << " = [x] + [y] + 2 [x, y] = " << qx + qy + 2.0 * cxy << "\n";
  return 0;
}
```

Output

```text
Ito of x^2 dx:            2.733069
Stratonovich of x^2 dx:   3.754421   (x_T^3 / 3 = 3.752384 in the limit)
Ito from n - 1 samples:   2.733069

[x] = 0.993383, [y] = 1.017693, [x, y] = -0.036456
[x + y] = 1.938163 = [x] + [y] + 2 [x, y] = 1.938163
```
The Stratonovich value 3.754421 is close to `x_T³/3 = 3.752384`, as the ordinary chain rule predicts; the Ito value 2.733069 is not, because `x` has quadratic variation. The covariation obeys the polarization identity `[x + y] = [x] + [y] + 2[x, y]` exactly, 1.938163 on both sides.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="the_forms_of_the_call.html">
    <span class="label">Previous</span>
    <span class="title">The forms of the call</span>
  </a>
  <a class="next-link" href="integrands_that_look_into_the_future.html">
    <span class="label">Next</span>
    <span class="title">Integrands that look into the future</span>
  </a>
</div>
<!-- apiref:nav:end -->
