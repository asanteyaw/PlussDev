<!-- generated from the_forms_of_the_call.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The forms of the call

[Back to Stochastic Integrals](../stochastic_01_integrals.md)


A path is a `Series`, and the integrals are methods of its `calc.antidiff` accessor. The integrand is a function `f(t, w)` called once, with the times and path values of every grid point stacked along the time axis, so it must act pointwise. The program shows a time-dependent integrand, an explicit non-uniform time grid, a path stored with time on the last axis, and the running integral.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const std::int64_t steps = 500;
  const pluss::Series dw = pluss::series::randn({steps, 2}, pluss::Config{}.with_dtype(f64), 3).mul(std::sqrt(1.0 / steps));
  const pluss::Series w = pluss::Series::zeros({1, 2}, f64).unite({dw.cumsum(0)}, 0);  // [steps + 1, 2]

  // f(t, w) = t * w: the integrand depends on time and on the path value.
  auto f = [](const pluss::Series& t, const pluss::Series& x) { return x.mul(t); };
  std::cout << std::fixed << std::setprecision(5);
  const auto total = w.calc.antidiff.ito(f, 0.0, 1.0).host<double>();
  std::cout << "uniform grid, Ito:          " << total[0] << "  " << total[1] << "\n";
  const auto strat = w.calc.antidiff.strat(f, 0.0, 1.0).host<double>();
  std::cout << "uniform grid, Stratonovich: " << strat[0] << "  " << strat[1] << "\n";

  // An explicit grid: the same samples, read as if time ran faster at the end. The increments of the path are
  // unchanged, only the times fed to f differ.
  const pluss::Series grid = pluss::Series::linspace(0.0, 1.0, steps + 1, f64).pow(2.0);
  const auto warped = w.calc.antidiff.ito(f, grid).host<double>();
  std::cout << "warped grid, Ito:           " << warped[0] << "  " << warped[1] << "\n";

  // The same path stored with time on the last axis: pass dim = 1.
  const pluss::Series wt = w.transpose(0, 1).contiguous();  // [2, steps + 1]
  const auto transposed = wt.calc.antidiff.ito(f, pluss::Series::linspace(0.0, 1.0, steps + 1, f64), 1).host<double>();
  std::cout << "time on the last axis:      " << transposed[0] << "  " << transposed[1] << "\n";

  // The running integral has the shape of the path and starts at zero.
  const pluss::Series running = w.calc.antidiff.ito_cumulative(f, 0.0, 1.0);
  std::cout << "\nrunning integral: shape [" << running.size(0) << ", " << running.size(1) << "], first row "
            << running.select(0, 0).host<double>()[0] << ", last row " << running.select(0, steps).host<double>()[0]
            << " (equals the total above)\n";
  return 0;
}
```

Output

```text
uniform grid, Ito:          -0.23277  -0.23616
uniform grid, Stratonovich: 0.00388  0.01356
warped grid, Ito:           -0.17236  -0.15878
time on the last axis:      -0.23277  -0.23616

running integral: shape [501, 2], first row 0.00000, last row -0.23277 (equals the total above)
```
The first and fourth lines are identical because the transposed path holds the same samples. The warped grid changes the times given to `f` but not the increments, so its value differs. The running integral has the shape of the path, starts at zero, and ends at the value of the total (−0.23277).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="the_family_in_between.html">
    <span class="label">Previous</span>
    <span class="title">The family in between</span>
  </a>
  <a class="next-link" href="values_that_were_not_simulated.html">
    <span class="label">Next</span>
    <span class="title">Values that were not simulated</span>
  </a>
</div>
<!-- apiref:nav:end -->
