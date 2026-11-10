<!-- generated from Method.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `linprog::Method`

The algorithm of the linear-programming solver: interior point or simplex.

`Method` is the `method` field of [`Options`](Options.md). `InteriorPoint` is the batched default: it solves all lanes together, returns dual certificates, and has the accuracy of an interior-point method. `Simplex` pivots between vertices: it returns exact vertex solutions, solves lanes one at a time (so a large batch is slower), and returns the optimal basis for warm-starting a related problem.

## Fields

### `InteriorPoint`

`enumerator`, default `default`.

The homogeneous self-dual interior-point method with Mehrotra predictor–corrector steps; batched, certificate-returning.

### `Simplex`

`enumerator`, default ``.

The simplex method: exact vertex solutions, one lane at a time, with an optimal basis returned and warm starts accepted.

## Guidance

`InteriorPoint` is the default and the right choice for batches; choose `Simplex` for exact vertices and warm starts.

## Practical Examples

### Example 1

The two methods on one problem: the same optimum, different by-products.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  // minimise -x0 - 2 x1   subject to   x0 + x1 <= 4,  x0 + 3 x1 <= 6,  x >= 0
  linprog::Problem problem;
  problem.c = pluss::series::make({-1.0, -2.0});
  problem.a_ub = pluss::series::make({{1.0, 1.0}, {1.0, 3.0}});
  problem.b_ub = pluss::series::make({4.0, 6.0});
  problem.lower = pluss::series::make({0.0, 0.0});

  linprog::Options simplex;
  simplex.method = linprog::Method::Simplex;
  const linprog::Result a = linprog::solve(problem);
  const linprog::Result b = linprog::solve(problem, simplex);
  out.writeln("InteriorPoint: objective =", a.objective.item<double>(), ", bases =", a.basis.size());
  out.writeln("Simplex      : objective =", b.objective.item<double>(), ", bases =", b.basis.size());
}
```

Output

```text
InteriorPoint: objective = -5.00000000 , bases = 0
Simplex      : objective = -5.00000000 , bases = 1
```

### Example 2

The exact vertex. The simplex returns a corner of the feasible region to rounding, where the interior point returns a point within its tolerance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(14));
  // minimise -x0 - 2 x1   subject to   x0 + x1 <= 4,  x0 + 3 x1 <= 6,  x >= 0
  linprog::Problem problem;
  problem.c = pluss::series::make({-1.0, -2.0});
  problem.a_ub = pluss::series::make({{1.0, 1.0}, {1.0, 3.0}});
  problem.b_ub = pluss::series::make({4.0, 6.0});
  problem.lower = pluss::series::make({0.0, 0.0});

  linprog::Options simplex;
  simplex.method = linprog::Method::Simplex;
  out.writeln("interior point error in x0:", std::abs(linprog::solve(problem).x.items<double>()[0] - 3.0));
  out.writeln("simplex error in x0       :", std::abs(linprog::solve(problem, simplex).x.items<double>()[0] - 3.0));
}
```

Output

```text
interior point error in x0: 0.00000000000157
simplex error in x0       : 0.00000000000000
```

### Example 3

A batch under each method. The interior point solves all lanes in one batched call; the simplex solves them one at a time. The answers agree.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // minimise -x0 - 2 x1   subject to   x0 + x1 <= 4,  x0 + 3 x1 <= 6,  x >= 0
  linprog::Problem problem;
  problem.c = pluss::series::make({-1.0, -2.0});
  problem.a_ub = pluss::series::make({{1.0, 1.0}, {1.0, 3.0}});
  problem.b_ub = pluss::series::make({4.0, 6.0});
  problem.lower = pluss::series::make({0.0, 0.0});

  problem.c = pluss::series::make({{-1.0, -2.0}, {-2.0, -1.0}, {-1.0, -1.0}});
  linprog::Options simplex;
  simplex.method = linprog::Method::Simplex;
  out.writeln("InteriorPoint objective =", linprog::solve(problem).objective.items<double>());
  out.writeln("Simplex       objective =", linprog::solve(problem, simplex).objective.items<double>());
}
```

Output

```text
InteriorPoint objective = [-5.000000, -8.000000, -4.000000]
Simplex       objective = [-5.000000, -8.000000, -4.000000]
```

## See Also

- [`Options`](Options.md): holds `method`
- [`solve`](solve.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Options.html">
    <span class="label">Next</span>
    <span class="title">Options</span>
  </a>
</div>
<!-- apiref:nav:end -->
