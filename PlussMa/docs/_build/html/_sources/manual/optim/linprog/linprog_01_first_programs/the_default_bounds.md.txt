<!-- generated from the_default_bounds.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The default bounds

[Back to First Programs](../linprog_01_first_programs.md)


If `lower` is left out, the variables are **free**. This surprises people who expect the usual `x >= 0`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  lp::Problem p;
  p.c = vec({1, -1});
  p.a_ub = mat({-1, -1, 1, 1}, 2, 2);  // x0 + x1 >= 2 ; x0 + x1 <= 4
  p.b_ub = vec({-2, 4});

  const auto describe = [](const char* label, const lp::Problem& problem) {
    const auto r = lp::solve(problem);
    const auto status = static_cast<lp::Status>(static_cast<int>(r.status.item<double>()));
    std::cout << std::left << std::setw(30) << label << (status == lp::Status::Optimal ? "Optimal" : status == lp::Status::Unbounded ? "Unbounded" : "other");
    if (status == lp::Status::Optimal) {
      const auto x = r.x.to_vec<double>();
      std::cout << std::fixed << std::setprecision(4) << "   x = (" << x[0] << ", " << x[1] << "), objective " << r.objective.item<double>();
    }
    std::cout << "\n";
  };
  describe("no bounds given (free)", p);
  lp::Problem nonnegative = p;
  nonnegative.lower = vec({0, 0});
  describe("lower = (0, 0)", nonnegative);
  return 0;
}
```

Output

```text
no bounds given (free)        Unbounded
lower = (0, 0)                Optimal   x = (0.0000, 4.0000), objective -4.0000
```
The same two rows and the same objective are unbounded with no bounds given and optimal, at `x = (0, 4)` with objective -4, when `lower = (0, 0)` is stated. Always set `lower` explicitly. The sparse solver of chapter 5 uses the other convention, `x >= 0` by default, so a model moved between the two needs its bounds checked.
