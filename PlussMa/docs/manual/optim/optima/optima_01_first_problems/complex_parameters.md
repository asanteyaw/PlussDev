<!-- generated from complex_parameters.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Complex parameters

[Back to Optima 1: Your First Constrained Problems](../optima_01_first_problems.md)

Some unknowns are complex numbers: the amplitude and phase of a signal, a filter coefficient, a reflection coefficient. `optima` accepts them directly. The **objective must still be a real number**, since there is no ordering on the complex numbers to minimize; the usual way to get one is the squared modulus, `abs().square()`.

## How it works

A complex parameter `z = x + iy` is optimized as two real coordinates, `x` and `y`, that the solver moves independently. You write the objective in terms of `z`; the solver differentiates it with respect to both parts, so you get exact derivatives without writing any. Two consequences follow, and both are visible in the programs below:

- The result keeps the dtype you started with. A complex start gives a complex `params` block, and the gradient in `evaluation.gradient` is complex too, with real part `dL/dx` and imaginary part `dL/dy`.
- A complex run is the same run as the one written with the real and imaginary parts as two real blocks. Program 3 checks this to the last digit.

Real and complex blocks can be mixed in one pack; only the complex blocks are treated this way.

## Program 1: A complex least-squares fit

Eight samples of a complex tone with an unknown complex amplitude `a`, `y_t = a e^{i omega t}` plus a small deterministic disturbance. The fit minimizes the sum of squared moduli of the residuals. The closed-form answer is `a = (1/N) sum conj(e_t) y_t`, and the program prints it next to the solver's.

```cpp
#include <pluss/ma.h>

#include <complex>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Complex = std::complex<double>;
  const auto c128 = pluss::ScalarType::Complex128;
  const double omega = 0.9;
  const Complex truth{1.5, -0.7};

  std::vector<Complex> tone, samples;
  for (int t = 0; t < 8; ++t) {
    tone.push_back(std::polar(1.0, omega * t));
    samples.push_back(truth * tone.back() + Complex{0.05 * std::sin(1.7 * t), 0.04 * std::cos(2.3 * t)});
  }
  const auto e = pluss::make_series(tone, {8}).to(c128);
  const auto y = pluss::make_series(samples, {8}).to(c128);

  const auto result = optim::optima([&](const pluss::Series &a) { return (y - a * e).abs().square().sum(); },
                                    pluss::make_series(std::vector<Complex>{{0.0, 0.0}}, {1}).to(c128));

  Complex exact{0.0, 0.0};
  for (int t = 0; t < 8; ++t) exact += std::conj(tone[t]) * samples[t];
  exact /= 8.0;
  const Complex found = result.params.at("x").to(pluss::pCPU, c128).host<Complex>()[0];
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "solver      " << found.real() << " " << (found.imag() < 0 ? "-" : "+") << " " << std::abs(found.imag()) << "i\n";
  std::cout << "closed form " << exact.real() << " " << (exact.imag() < 0 ? "-" : "+") << " " << std::abs(exact.imag()) << "i\n";
  std::cout << "converged: " << std::boolalpha << result.converged << "\n";
}
```

Output

```text
solver      1.49601 - 0.69824i
closed form 1.49601 - 0.69824i
converged: true
```

The solver and the closed form agree to the printed digits, and both are close to the true amplitude `1.5 - 0.7i`, off by the size of the disturbance. The imaginary part is found as well as the real part: the answer is the whole complex number, not its real projection.

## Program 2: A complex constraint and a modulus constraint

Constraints are stated on the complex parameters in the same way. An **equality** may be complex: it is enforced for the real and the imaginary part of the residual. An **inequality** must be real, because `<=` has no meaning for complex numbers; a modulus bound is the natural example.

The first problem minimizes `|z0|^2 + |z1|^2` subject to `z0 + z1 = 3 - 2i`, whose answer is half of the right-hand side in each unknown. The second finds the point of the unit disc `|z| <= 1` nearest `3 + 4i`, which is `(3 + 4i) / 5 = 0.6 + 0.8i`.

```cpp
#include <pluss/ma.h>

#include <complex>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  using Complex = std::complex<double>;
  const auto c128 = pluss::ScalarType::Complex128;
  const auto cvec = [&](std::vector<Complex> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(c128); };
  std::cout << std::fixed << std::setprecision(5);

  const auto rhs = cvec({{3.0, -2.0}});
  const auto sum_constraint = optim::constraints().nl_eq([&](const Pack &p) { return p.at("x").select(0, 0) + p.at("x").select(0, 1) - rhs.select(0, 0); });
  const auto split = optim::optima([](const pluss::Series &z) { return z.abs().square().sum(); }, cvec({{1.0, 1.0}, {-1.0, 0.5}}), {}, sum_constraint);
  const auto z = split.params.at("x").to(pluss::pCPU, c128).host<Complex>();
  std::cout << "equality:   z0 = " << z[0] << "  z1 = " << z[1] << "  (each half of 3 - 2i)\n";

  const auto target = cvec({{3.0, 4.0}});
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").abs().square().sum() - 1.0; });
  const auto nearest = optim::optima([&](const pluss::Series &w) { return (w - target.select(0, 0)).abs().square().sum(); }, cvec({{0.0, 0.0}}), {}, disc);
  const Complex w = nearest.params.at("x").to(pluss::pCPU, c128).host<Complex>()[0];
  std::cout << "unit disc:  w = " << w << "  |w| = " << std::abs(w) << "  (the point 0.6 + 0.8i)\n";
}
```

Output

```text
equality:   z0 = (1.50000,-1.00000)  z1 = (1.50000,-1.00000)  (each half of 3 - 2i)
unit disc:  w = (0.60000,0.80000)  |w| = 1.00000  (the point 0.6 + 0.8i)
```

The equality splits the right-hand side evenly, `1.5 - 1i` for each unknown, and the disc problem lands on the boundary at `0.6 + 0.8i` with `|w| = 1`: the constraint is active, as it must be, because the target lies outside the disc.

## Program 3: The same problem with real parts and imaginary parts

The treatment is not an approximation, and it can be checked. The problem is `|z^2 - 1|^2`, whose minima are `z = 1` and `z = -1`, from a start with both parts nonzero. Written with the real and imaginary parts as two real blocks, it is `(x^2 - y^2 - 1)^2 + (2xy)^2`. The program solves both forms and prints the answers and the iteration counts.

```cpp
#include <pluss/ma.h>

#include <complex>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  using Complex = std::complex<double>;
  const auto c128 = pluss::ScalarType::Complex128;
  const auto f64 = pluss::ScalarType::Float64;
  std::cout << std::fixed << std::setprecision(8);

  const auto complex_run = optim::optima([](const pluss::Series &z) { return (z.square() - 1.0).abs().square().sum(); },
                                         pluss::make_series(std::vector<Complex>{{0.6, 0.4}}, {1}).to(c128));

  const Pack real_start({{"x", pluss::make_series(std::vector<double>{0.6}, {1}).to(f64)}, {"y", pluss::make_series(std::vector<double>{0.4}, {1}).to(f64)}});
  const auto real_run = optim::optima(
      [](const Pack &p) {
        const auto x = p.at("x"), y = p.at("y");
        return (x.square() - y.square() - 1.0).square().sum() + (x * y * 2.0).square().sum();
      },
      real_start);

  const Complex z = complex_run.params.at("x").to(pluss::pCPU, c128).host<Complex>()[0];
  std::cout << "complex run: " << z.real() << " " << z.imag() << "  iterations " << complex_run.iteration << "\n";
  std::cout << "split run:   " << real_run.params.at("x").host<double>()[0] << " " << real_run.params.at("y").host<double>()[0] << "  iterations " << real_run.iteration << "\n";
}
```

Output

```text
complex run: 0.99999920 0.00000022  iterations 7
split run:   0.99999920 0.00000022  iterations 7
```

Both runs finish in the same number of iterations at the same point, to every printed digit, and the point is the minimum `z = 1`.

## Program 4: What is refused

Four things are refused, each with a message that says why, because each has no sensible meaning for a complex unknown: a **complex objective**, a **complex inequality residual**, **bounds** on a complex parameter (there is no ordering to bound by; bound the real and imaginary parts as separate real parameters, or constrain the modulus as in Program 2), and **linear constraints** (the matrix would have to act on the real and imaginary parts at once; write the constraint as a nonlinear equality or inequality). These are reported when the call is made, before any iteration.

```cpp
#include <pluss/ma.h>

#include <complex>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  using Complex = std::complex<double>;
  const auto c128 = pluss::ScalarType::Complex128;
  const auto f64 = pluss::ScalarType::Float64;
  const auto start = pluss::make_series(std::vector<Complex>{{1.0, 1.0}, {2.0, -1.0}}, {2}).to(c128);
  const auto loss = [](const pluss::Series &z) { return z.abs().square().sum(); };
  const auto report = [](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      std::cout << text << "\n";
    }
  };

  report([&] { (void)optim::optima([](const pluss::Series &z) { return z.sum(); }, start); });
  report([&] {
    const auto c = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").sum(); });
    (void)optim::optima(loss, start, {}, c);
  });
  report([&] {
    const auto c = optim::constraints().ln_eq(pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2}).to(f64), pluss::make_series(std::vector<double>{1.0}, {1}).to(f64));
    (void)optim::optima(loss, start, {}, c);
  });
  report([&] {
    optim::ConstrainedProblem problem;
    problem.objective.autodiff = [&](const pluss::Series &z) { return loss(z); };
    problem.bounds.lower = Pack({{"x", pluss::make_series(std::vector<double>{-1.0, -1.0}, {2}).to(f64)}});
    (void)optim::optima(problem, Pack({{"x", start}}));
  });
}
```

Output

```text
optim::optima: the objective must be real-valued when the parameters are complex (take abs, real or imag of it)
optim::optima: the inequality constraint must be real-valued when the parameters are complex (take abs, real or imag of it)
optim::optima: linear constraints are not supported when a parameter is complex (the matrix would act on the real and imaginary parts at once); write the constraint as a nonlinear equality or inequality
optim::optima: bounds on the complex parameter 'x' are not supported: there is no ordering on the complex numbers (bound its real and imaginary parts as separate real parameters, or constrain abs(z))
```

## Decisions and pitfalls

- **Make the objective real with `abs().square()`, not `square()`.** `z.square()` is `z * z`, which is still complex; `z.abs().square()` is the real quantity `x^2 + y^2`.
- **The gradient of a complex block is `dL/dx + i dL/dy`.** If you give a hand-written gradient for a complex block (the analytic form of Chapter 1), return that, not the derivative with respect to `z` in the holomorphic sense.
- **A mixed pack is fine.** Real blocks keep their dtype and their bounds. Linear constraints are the exception: with a complex block anywhere in the problem they are refused, because their matrix acts on all of the unknowns at once.
- **Constraint Jacobians reported in `evaluation` are with respect to the real and imaginary parts,** with a trailing axis of size 2, since the constraint rows are expanded the same way.

Where next: [Chapter 2](../optima_02_reading_results.md) reads results in depth.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="blocks_values_and_reading_the_result.html">
    <span class="label">Previous</span>
    <span class="title">Blocks, problems as values, gradients and reading the result</span>
  </a>
</div>
<!-- apiref:nav:end -->
