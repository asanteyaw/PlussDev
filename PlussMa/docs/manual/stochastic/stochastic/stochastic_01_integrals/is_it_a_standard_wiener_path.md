<!-- generated from is_it_a_standard_wiener_path.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Is it a standard Wiener path?

[Back to Stochastic Integrals](../stochastic_01_integrals.md)


Ito and Stratonovich values are the standard ones only when the integrator has quadratic variation equal to elapsed time. Every result from the wrapper carries `quadratic_variation_ratio` and a list of warnings. The program feeds it a standard path, the same path scaled by 3, and a straight line.

```cpp
#include <pluss/ma.h>

#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series like = pluss::make_series(0.0).to(f64).reshape({1});
  auto identity = [](double, const pluss::Series& w) { return w; };

  const auto standard = sde::simulate_wiener_path(like, 0.0, 1.0, 2000, /*seed=*/3);
  sde::WienerPath tripled = standard;
  tripled.w = standard.w.mul(3.0);                                  // volatility 3
  sde::WienerPath smooth = standard;
  smooth.w = pluss::Series::linspace(0.0, 1.0, 2001, f64).unsqueeze(1);  // a straight line

  for (const auto& item : {std::pair<const char*, sde::WienerPath>{"standard path", standard},
                           {"path times 3", tripled},
                           {"straight line", smooth}}) {
    const auto r = sde::ito_integral(identity, item.second);
    std::cout << item.first << ": quadratic_variation_ratio = " << r.quadratic_variation_ratio << ", warnings = "
              << r.warnings.size() << "\n";
    for (const auto& w : r.warnings) std::cout << "    " << w << "\n";
  }
  return 0;
}
```

Output

```text
standard path: quadratic_variation_ratio = 0.978837, warnings = 0
path times 3: quadratic_variation_ratio = 8.80953, warnings = 1
    the path's realized quadratic variation is 8.809532 times the elapsed time; a standard Wiener path gives about 1, so this is not a standard Brownian path (another volatility, a smooth path, or jumps) and Ito and Stratonovich values are not the standard ones
straight line: quadratic_variation_ratio = 0.0005, warnings = 1
    the path's realized quadratic variation is 0.000500 times the elapsed time; a standard Wiener path gives about 1, so this is not a standard Brownian path (another volatility, a smooth path, or jumps) and Ito and Stratonovich values are not the standard ones
```
The standard path gives a ratio of 0.978837 and no warning. The tripled path gives 8.80953, close to `3² = 9`, and the straight line 0.0005; both draw a warning. Nothing is thrown, because a scaled or smooth path is a legitimate integrator; the warning is there so a wrong path is not mistaken for a standard one.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="integrands_that_look_into_the_future.html">
    <span class="label">Previous</span>
    <span class="title">Integrands that look into the future</span>
  </a>
</div>
<!-- apiref:nav:end -->
