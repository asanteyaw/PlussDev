<!-- generated from the_skorokhod_integral.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The Skorokhod integral

[Back to Malliavin Calculus](../stochastic_05_malliavin.md)


For an adapted integrand the Skorokhod integral reduces to the Ito sum. For an anticipating one it does not: with `u_i = W_T` for every `i`, the Ito sum is not defined, but `δ(u) = W_T² − T`. The second check runs 20000 paths at once.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const std::int64_t n = 50, paths = 20000;
  const double dt = 1.0 / n;
  std::cout << std::fixed << std::setprecision(5);

  // One path: u_i = W_{t_i} (adapted) gives the Ito sum.
  const pluss::Series dw = pluss::series::randn({n}, pluss::Config{}.with_dtype(f64), 8).mul(std::sqrt(dt));
  const auto adapted = [](const pluss::Series& inc) { return inc.cumsum(0).sub(inc); };  // W at the left end of each step
  const double sk = dw.calc.diff.skorokhod(adapted, dt).item<double>();
  const double ito = adapted(dw).mul(dw).sum().item<double>();
  std::cout << "adapted integrand: Skorokhod " << sk << ", Ito sum " << ito << "\n";

  // Anticipating integrand u_i = W_T for every i, over many paths.
  const pluss::Series dws = pluss::series::randn({n, paths}, pluss::Config{}.with_dtype(f64), 9).mul(std::sqrt(dt));
  const auto anticipating = [](const pluss::Series& inc) { return inc.sum(0).unsqueeze(0).expand({inc.size(0), inc.size(1)}); };
  const pluss::Series delta = dws.calc.diff.skorokhod(anticipating, dt);
  const pluss::Series wT = dws.sum(0);
  std::cout << "anticipating u = W_T: mean |delta(u) - (W_T^2 - 1)| = "
            << delta.sub(wT.mul(wT).sub(1.0)).abs().mean().item<double>() << "\n";
  return 0;
}
```

Output

```text
adapted integrand: Skorokhod -0.60934, Ito sum -0.60934
anticipating u = W_T: mean |delta(u) - (W_T^2 - 1)| = 0.00000
```
The first line shows the two sums agree on an adapted integrand, −0.60934 on both sides. The second shows the anticipating identity: the mean absolute difference between `δ(u)` and `W_T² − 1` is 0.00000 to the printed digits. The cost is one reverse pass per increment, so use it on the grid sizes of a test or a sensitivity, not on millions of steps.
