<!-- generated from solving_equations.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Solving equations

[Back to Root Finding: Solving Equations](../roots_01_solving_equations.md)

A root of `f` is a value `x` with `f(x) = 0`. Pluss solves scalar equations in **batches**: every solver advances all lanes of a `Series` together with ordinary array operations, so one call solves a single equation or a whole parameter sweep with a different equation in each lane. The loop stops when the *slowest* lane is done, so every lane gets the same iteration count.

The five programs cover choosing and using a method. A first root with bisection shows the floor of rounding. A batch of six cubics solved in one call is the form to use for a sweep or a calibration of many instruments, with no host loop to pay for. The methods are compared as rates: bisection gains about one binary digit per step, Illinois accelerates, Newton with the exact derivative converges quadratically. Tolerances, the check cadence and brackets are explained, with the warning that a bracketed solver can return a lane that is *not* a root. The last program is a fourth-order solver that also keeps the derivative of the root with respect to a parameter the function captured.

## Program 1: A first root

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const roots::Fn f = [](const Series& x) { return x.pow(3.0) - x * 2.0 - 5.0; };
  const auto r = roots::bisect(f, vec({2.0}), vec({3.0}));
  std::cout << std::setprecision(15) << "root       : " << as_f64(r.root) << "\n";
  std::cout << std::scientific << std::setprecision(2) << "residual   : " << as_f64(r.residual) << "\n";
  std::cout << "iterations : " << r.iterations << "\n";
  std::cout << "bracketed  : " << (r.bracketed ? "yes" : "no") << "\n";
}
```

Output

```text
root       : 2.09455148154233
residual   : 3.55e-15
iterations : 100
bracketed  : yes
```
No tolerance was set, so all 100 bisection steps ran; the residual of 3.6e-15 is the rounding floor of the function near the root. With a tolerance the same answer takes far fewer steps (Program 4).

## Program 2: Many equations at once

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const Series k = vec({0.5, 2.0, 6.0, 24.0, 60.0, 120.0});
  const roots::Fn f = [&](const Series& x) { return x.pow(3.0) - x - k; };
  const auto r = roots::illinois(f, k * 0.0 + 1.0, k * 0.0 + 10.0, {200, 0.0, 1e-14, 8});
  const auto x = r.root.host<double>();
  const auto kv = k.host<double>();
  const auto res = r.residual.host<double>();
  std::cout << std::left << std::setw(8) << "k" << std::setw(20) << "root" << "residual\n";
  for (std::size_t i = 0; i < x.size(); ++i)
    std::cout << std::left << std::fixed << std::setprecision(1) << std::setw(8) << kv[i] << std::setprecision(12) << std::setw(20) << x[i] << std::scientific << std::setprecision(1) << res[i] << "\n";
  std::cout << "iterations: " << r.iterations << " (all lanes together)\n";
}
```

Output

```text
k       root                residual
0.5     1.191487883953      4.4e-16
2.0     1.521379706805      0.0e+00
6.0     2.000000000000      0.0e+00
24.0    3.000000000000      0.0e+00
60.0    4.000000000000      0.0e+00
120.0   5.000000000000      0.0e+00
iterations: 16 (all lanes together)
```
One call, six different cubics, one set of array operations per iteration. The loop stops when the *slowest* lane is done, so every lane gets the same iteration count. This is the form to use for a sweep or a calibration of many instruments; there is no per-lane host loop to pay for.

## Program 3: The methods compared

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const roots::Fn f = [](const Series& x) { return x.exp() - 5.0; };
  const roots::Fn df = [](const Series& x) { return x.exp(); };
  std::cout << std::left << std::setw(12) << "iterations" << std::setw(12) << "bisect" << std::setw(12) << "illinois" << std::setw(18)
            << "newton (exact)" << std::setw(18) << "newton (diff.)" << "secant\n";
  for (std::int64_t k : {2, 4, 6, 8, 12}) {
    roots::Options o;
    o.max_iterations = k;
    const auto b = roots::bisect(f, vec({0.0}), vec({3.0}), o);
    const auto il = roots::illinois(f, vec({0.0}), vec({3.0}), o);
    const auto nx = roots::newton(f, vec({1.0}), df, o);
    const auto nd = roots::newton(f, vec({1.0}), {}, o);
    const auto sc = roots::secant(f, vec({1.0}), vec({2.0}), o);
    std::cout << std::left << std::setw(12) << k << std::scientific << std::setprecision(1) << std::setw(12) << std::abs(as_f64(b.residual))
              << std::setw(12) << std::abs(as_f64(il.residual)) << std::setw(18) << std::abs(as_f64(nx.residual)) << std::setw(18)
              << std::abs(as_f64(nd.residual)) << std::abs(as_f64(sc.residual)) << "\n";
  }
}
```

Output

```text
iterations  bisect      illinois    newton (exact)    newton (diff.)    secant
2           1.5e+00     2.2e+00     1.2e-01           1.2e-01           1.1e-01
4           7.8e-02     4.3e-01     2.2e-07           2.2e-07           7.8e-05
6           3.9e-02     1.1e-03     0.0e+00           0.0e+00           4.3e-13
8           9.5e-03     1.2e-07     0.0e+00           0.0e+00           0.0e+00
12          3.0e-04     0.0e+00     0.0e+00           0.0e+00           0.0e+00
```
The table reads as rates. Bisection gains about one binary digit per step and its residual does not fall evenly, because the midpoint can land on either side of the root. Illinois starts slower here (its early steps are long) and then accelerates sharply. Newton, with the exact derivative or with the built-in central difference, is quadratic: the number of correct digits roughly doubles each step, and it is at rounding level by step 6 (the `0.0e+00` entries are residuals below 1e-16). The secant method, which needs no derivative, is nearly as fast. Use a bracketed method when you have a bracket and want certainty, Newton when you can supply or cheaply difference the derivative, and the secant method when you have neither.

## Program 4: Tolerances, check cadence and brackets

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const roots::Fn f = [](const Series& x) { return x.exp() - 5.0; };
  std::cout << std::left << std::setw(42) << "setting" << std::setw(12) << "iterations" << "|residual|\n";
  auto show = [&](const char* label, const roots::Result& r) {
    std::cout << std::left << std::setw(42) << label << std::setw(12) << r.iterations << std::scientific << std::setprecision(1) << std::abs(as_f64(r.residual)) << "\n";
  };
  show("illinois, no tolerance, 30 iterations", roots::illinois(f, vec({0.0}), vec({3.0}), {30, 0.0, 0.0, 8}));
  show("illinois, f_tolerance 1e-8, every 1", roots::illinois(f, vec({0.0}), vec({3.0}), {30, 0.0, 1e-8, 1}));
  show("illinois, f_tolerance 1e-8, every 8", roots::illinois(f, vec({0.0}), vec({3.0}), {30, 0.0, 1e-8, 8}));
  show("bisect, x_tolerance 1e-6, every 1", roots::bisect(f, vec({0.0}), vec({3.0}), {60, 1e-6, 0.0, 1}));

  const roots::Fn g = [](const Series& x) { return x.square() + 1.0; };  // no real root
  const auto bad = roots::bisect(g, vec({-1.0}), vec({1.0}));
  std::cout << "\nx^2 + 1 on [-1, 1]: bracketed = " << (bad.bracketed ? "yes" : "no") << ", residual " << std::fixed << std::setprecision(3) << as_f64(bad.residual) << "\n";
}
```

Output

```text
setting                                   iterations  |residual|
illinois, no tolerance, 30 iterations     30          0.0e+00
illinois, f_tolerance 1e-8, every 1       9           1.2e-11
illinois, f_tolerance 1e-8, every 8       16          0.0e+00
bisect, x_tolerance 1e-6, every 1         22          7.5e-07

x^2 + 1 on [-1, 1]: bracketed = no, residual 2.000
```
The third row shows the cadence: the same tolerance, checked every 8 iterations instead of every iteration, stops at 16 instead of 9. When `bracketed` is false the answer for that lane is not a root: always read it for bracketed solvers, and always read `residual`.

## Program 5: A fourth-order solver and the derivative of the root

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  Series p = pluss::series::make(2.0);
  p.tracks_slope_(true);
  const auto r = roots::householder(
      [p](const Series& x) {
        return roots::Derivatives{x.square() - p, x * 2.0, pluss::series::full_like(x, 2.0), pluss::series::zeros_like(x)};
      },
      pluss::series::make(1.0), {12, 1e-13, 1e-13, 1});
  std::cout << std::setprecision(15) << "root        : " << as_f64(r.root) << "  (sqrt 2 = " << std::sqrt(2.0) << ")\n";
  std::cout << "iterations  : " << r.iterations << "\n";
  r.root.reverse();
  std::cout << "d root / dp : " << as_f64(p.sensitivity()) << "  (exact " << 0.5 / std::sqrt(2.0) << ")\n";
}
```

Output

```text
root        : 1.4142135623731  (sqrt 2 = 1.4142135623731)
iterations  : 4
d root / dp : 0.353553390593274  (exact 0.353553390593274)
```
`householder` converges in four steps here. It also keeps the dependence of the root on any Series the function captured: marking the parameter with `tracks_slope_(true)` and calling `reverse()` on the root gives d(root)/dp from the implicit function theorem, whatever the starting guess. The root is rebuilt from the final evaluation, so the history of the iterations is not stored.

## When a fast method fails and a bracket does not

The fast methods (Newton, secant) need a good starting point; the bracketed methods (bisection, Illinois) need a sign change but then cannot fail. A batch makes the difference vivid: one call solves the same equation from 41 starting points (or 41 brackets), and the table counts the lanes that end at a root. The equation `x³ − 2x + 2 = 0` is the classic trap: it has one real root near −1.77, and Newton's method started near 0 or 1 cycles between them forever.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const roots::Fn f = [](const Series& x) { return x * x * x - x * 2.0 + 2.0; };
  const roots::Fn df = [](const Series& x) { return x * x * 3.0 - 2.0; };
  std::vector<double> starts;
  for (int i = 0; i <= 40; ++i) starts.push_back(-2.0 + 0.1 * i);  // -2 ... 2

  roots::Options o;
  o.max_iterations = 100;
  o.x_tolerance = 1e-12;
  const auto newton = roots::newton(f, vec(starts), df, o);
  const auto residual = newton.residual.host<double>();
  int converged = 0;
  for (std::size_t i = 0; i < residual.size(); ++i) {
    if (std::fabs(residual[i]) < 1e-9) ++converged;
  }

  // The same equation bracketed on [-3, 3] for all 41 lanes (identical brackets): one root, no way to miss it.
  std::vector<double> lo(41, -3.0), hi(41, 3.0);
  const auto bis = roots::bisect(f, vec(lo), vec(hi), o);
  int bisected = 0;
  for (double r : bis.residual.host<double>()) if (std::fabs(r) < 1e-9) ++bisected;

  std::cout << "starting points from -2.0 to 2.0 in steps of 0.1 (41 lanes in one call)\n";
  std::cout << "Newton lanes that reached a root (|f| < 1e-9): " << converged << " of 41\n";
  std::cout << "bisection lanes that reached a root:           " << bisected << " of 41  (bracketed: " << (bis.bracketed ? "yes" : "no") << ")\n";
}
```

Output

```text
starting points from -2.0 to 2.0 in steps of 0.1 (41 lanes in one call)
Newton lanes that reached a root (|f| < 1e-9): 33 of 41
bisection lanes that reached a root:           41 of 41  (bracketed: yes)
```

Newton reaches a root from 33 of the 41 starting points and fails from 8, which is the figure that matters: it is fast when it works, and from the lanes near the two-cycle (where the iteration bounces between 0 and 1 instead of settling) it never reaches the root in 100 iterations, with no warning but a large residual. The bracketed method reaches the root from every lane, because a sign change on an interval is a guarantee, at the price of being slower. The practical rule is to use a bracketed method when you can bracket the root and then, if speed matters, to polish with Newton from the bracketed answer; and always read the residual.

## Decisions and pitfalls

- **Set a tolerance.** Without one, bisection runs all its steps (100) and stops at the rounding floor of the function near the root.
- **Read `bracketed` and `residual`.** When `bracketed` is false the answer for that lane is not a root; always read the residual too.
- **The check cadence changes the iteration count.** The same tolerance checked every 8 iterations stops at 16 instead of 9.
- **Use a batch for a sweep.** One call with a different equation per lane avoids a per-lane host loop.
- **The derivative of a root is free with the right solver.** Marking the parameter and calling `reverse()` gives `d(root)/dp` by the implicit function theorem, whatever the starting point.

Where next: [Quadratic programming](../../qp.md) is next.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
