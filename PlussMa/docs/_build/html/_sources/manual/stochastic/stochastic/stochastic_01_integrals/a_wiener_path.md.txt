<!-- generated from a_wiener_path.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A Wiener path

[Back to Stochastic Integrals](../stochastic_01_integrals.md)


`simulate_wiener_path` draws a path, or a batch of independent paths, on a uniform grid starting from zero. The shape of the `like` argument is the batch shape, so one call makes any number of independent paths. The program prints the realized quadratic variation (the sum of squared increments) at several step counts.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  // `like` fixes the batch shape and dtype: three independent paths.
  const pluss::Series like = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0}, {3}).to(f64);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "steps   path 0   path 1   path 2   (realized quadratic variation on [0, 1])\n";
  for (const std::int64_t steps : {10, 100, 1000, 10000}) {
    const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, steps, /*seed=*/2024);
    const pluss::Series qv = path.w.antidiff().quadratic_variation(0);
    const auto v = qv.host<double>();
    std::cout << std::setw(5) << steps << "   " << v[0] << "   " << v[1] << "   " << v[2] << "\n";
  }

  const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, 1000, /*seed=*/2024);
  std::cout << "\nsamples per path: " << path.w.size(0) << " (steps + 1), batch shape [" << path.w.size(1) << "]\n";
  std::cout << "first sample is the start: " << path.w.select(0, 0).host<double>()[0] << "\n";
  return 0;
}
```

Output

```text
steps   path 0   path 1   path 2   (realized quadratic variation on [0, 1])
   10   1.0708   0.5537   1.1157
  100   0.8176   0.9806   1.0375
 1000   0.9455   0.9688   1.0377
10000   1.0027   1.0172   1.0117

samples per path: 1001 (steps + 1), batch shape [3]
first sample is the start: 0.0000
```
The path has `steps + 1` samples because the first is the start. The quadratic variation is random at 10 steps (between 0.55 and 1.12 across the three paths) and settles toward the elapsed time, 1, as the grid is refined; at 10000 steps the three paths give 1.0027, 1.0172 and 1.0117. This number is the reason the two integrals differ, and it is what the diagnostics of Program 7 compare against.
