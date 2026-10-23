<!-- generated from a_direct_call.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A direct call

[Back to The Nelder–Mead Minimiser](../dfo_06_nelder_mead.md)


```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto rosenbrock = [](const pluss::Series& x) {
    const auto a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
  };
  const auto out = dfo::nelder_mead_minimizer(rosenbrock, vec({-1.2, 1.0}));
  std::cout << std::fixed << std::setprecision(5) << "minimizer  : " << out.minimizer.select(0, 0).item<double>() << ", "
            << out.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum    : " << out.minimum << "\n";
  std::cout << "status     : " << (out.status == dfo::SolverStatus::Converged ? "Converged" : "budget") << "\n";
  std::cout << "iterations : " << out.diagnostics.iterations << "\n";
  std::cout << "evaluations: " << out.diagnostics.evaluations << "\n";
  std::cout << "last step  : " << out.diagnostics.last_step << "\n";
}
```

Output

```text
minimizer  : 1.00002, 1.00004
minimum    : 8.18e-10
status     : Converged
iterations : 85
evaluations: 159
last step  : adjust_inner
```
The accuracy is about 1e-5 in the variables: a function value of 1e-9 and a tolerance of 1e-4 on the simplex size. For more digits, tighten the tolerances (Program 2) or polish with another method (Chapter 5).
