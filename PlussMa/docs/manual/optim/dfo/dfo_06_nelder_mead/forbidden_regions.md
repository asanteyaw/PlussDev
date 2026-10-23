<!-- generated from forbidden_regions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Forbidden regions

[Back to The Nelder–Mead Minimiser](../dfo_06_nelder_mead.md)


An objective can return `+infinity` for a point that is not allowed. The simplex sees a very bad value and moves away.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto f = [](const pluss::Series& x) -> double {
    const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
    if (a * a + b * b > 4.0) return std::numeric_limits<double>::infinity();
    return (a - 3) * (a - 3) + (b - 3) * (b - 3);
  };
  const auto o = dfo::nelder_mead_minimizer(f, vec({0.0, 0.0}));
  std::cout << std::fixed << std::setprecision(4) << "minimizer : " << o.minimizer.select(0, 0).item<double>() << ", "
            << o.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << "minimum   : " << o.minimum << "  (exact 5.0294)\n";
  std::cout << "evaluations: " << o.diagnostics.evaluations << "\n";

  auto bad = [](const pluss::Series&) -> double { return std::nan(""); };
  try {
    (void)dfo::nelder_mead_minimizer(bad, vec({0.0, 0.0}));
  } catch (const std::exception&) {
    std::cout << "a NaN objective is rejected with an error\n";
  }
}
```

Output

```text
minimizer : 1.3744, 1.4529
minimum   : 5.0360  (exact 5.0294)
evaluations: 252
a NaN objective is rejected with an error
```
The result is the nearest point of the disc to about three digits. A simplex pressed against a wall stops a little short of the exact optimum, because most of its trial points fall in the forbidden region and are wasted. For a precise answer on a constraint boundary, use pattern search (Chapter 3). NaN and negative infinity are rejected with an error, since they cannot be ordered.
