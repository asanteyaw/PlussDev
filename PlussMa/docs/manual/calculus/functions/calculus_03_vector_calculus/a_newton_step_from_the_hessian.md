<!-- generated from a_newton_step_from_the_hessian.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Worked problem: Newton's method on a batch of starts

[Back to Vector Calculus](../calculus_03_vector_calculus.md)

Newton's method for minimising a function repeats x ← x − H⁻¹ g, where g is the gradient and H the Hessian at the current point. Both come directly from the operators, and because they are functions of arrays, one call advances any number of starting points together.

The function is the Rosenbrock valley, f(x, y) = (1 − x)² + 100 (y − x²)², whose minimum is at (1, 1) and whose curved floor is a standard stress test. The program starts from three points at once, solves the 2 × 2 Newton system for each by Cramer's rule, and prints the function value and the iterate at every step.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function rosenbrock = math::function({"x", "y"}, [](math::Function x, math::Function y) {
    return (1.0 - x).pow(2) + 100.0 * (y - x.pow(2)).pow(2);
  });
  const math::Function gradient = operators::gradient()(rosenbrock);
  const math::Function hessian = operators::hessian()(rosenbrock);

  auto x = pluss::make_series({-1.2, 0.5, 2.0});
  auto y = pluss::make_series({1.0, 0.5, 3.0});
  for (int step = 0; step <= 6; ++step) {
    const auto g = gradient({x, y});   // shape [3, 2]
    const auto h = hessian({x, y});    // shape [3, 2, 2]
    const auto gx = g.select(1, 0), gy = g.select(1, 1);
    const auto a = h.select(1, 0).select(1, 0), b = h.select(1, 0).select(1, 1), d = h.select(1, 1).select(1, 1);
    const auto det = a.mul(d).sub(b.mul(b));
    const auto dx = d.mul(gx).sub(b.mul(gy)).div(det);   // H^{-1} g, by Cramer's rule
    const auto dy = a.mul(gy).sub(b.mul(gx)).div(det);
    out.writeln("step", step, " f", rosenbrock({x, y}).items<double>(), " x", x.items<double>(), " y", y.items<double>());
    x = x.sub(dx);
    y = y.sub(dy);
  }
}
```

Output

```text
step 0  f [24.200000, 6.500000, 101.000000]  x [-1.200000, 0.500000, 2.000000]  y [1.000000, 0.500000, 3.000000]
step 1  f [4.731884, 0.260309, 0.990075]  x [-1.175281, 0.489796, 1.995025]  y [1.380674, 0.239796, 3.980100]
step 2  f [1411.845179, 6.239915, 96.107526]  x [0.763115, 0.989592, 1.004901]  y [-3.175034, 0.729496, 0.029483]
step 3  f [0.055966, 0.000104, 0.000024]  x [0.763430, 0.989796, 1.004877]  y [0.582825, 0.979696, 1.009777]
step 4  f [0.313189, 0.000001, 0.000000]  x [0.999995, 1.000000, 1.000000]  y [0.944027, 0.999896, 0.999976]
step 5  f [0.000000, 0.000000, 0.000000]  x [0.999996, 1.000000, 1.000000]  y [0.999991, 1.000000, 1.000000]
step 6  f [0.000000, 0.000000, 0.000000]  x [1.000000, 1.000000, 1.000000]  y [1.000000, 1.000000, 1.000000]
```

All three starts reach (1, 1) in six steps or fewer, and the printed values show the same pattern for each: a large first reduction in `f`, a rise at the second step, when the unsafeguarded step overshoots (from 4.73 to 1411.8 for the classic start (−1.2, 1), from 0.26 to 6.24 and from 0.99 to 96.1 for the others), and then the quadratic convergence of Newton's method, where the number of correct digits roughly doubles with each step. The gradient and Hessian calls evaluate all three starts in a single call each, and the arithmetic that follows is ordinary `Series` arithmetic on the three-element columns.

The derivatives are exact because they were derived from the formula, not estimated, which is what makes a method that needs second derivatives practical here. This Newton iteration is not safeguarded, and the first start shows it: f jumps from 4.73 to 1411.8 before the method recovers. A production solver adds a line search or a trust region; the optimization guides document the library's safeguarded solvers.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="batches_and_broadcasting.html">
    <span class="label">Previous</span>
    <span class="title">Batches and broadcasting</span>
  </a>
</div>
<!-- apiref:nav:end -->
