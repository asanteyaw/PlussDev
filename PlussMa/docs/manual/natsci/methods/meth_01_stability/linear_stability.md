# Linear stability of an equilibrium

[Back to Methods 1: Stability and Branches](../meth_01_stability.md)

You have a model {math}`\mathrm{d}y/\mathrm{d}t = f(y)` and a state where {math}`f = 0`, and you want to know whether a small disturbance dies out, and how the answer changes when a parameter moves. This page counts the unstable directions of an equilibrium from its Jacobian without computing a single eigenvalue, so the count can be made for a whole sweep of parameters in one call, and shows how a pair of eigenvalues crossing the imaginary axis (the start of an oscillation) appears in the count.

## How it works

Near an equilibrium {math}`y^*` a disturbance {math}`\delta = y - y^*` obeys {math}`\dot\delta = J\delta` with the Jacobian {math}`J = \partial f/\partial y` at {math}`y^*`. The equilibrium attracts if every eigenvalue of {math}`J` has a negative real part. The eigenvalues are the roots of the characteristic polynomial {math}`\det(\lambda I - J) = \sum_k c_k\lambda^{n-k}`, whose coefficients follow from traces of matrix powers by the Faddeev-LeVerrier recursion

```{math}
M_k = J M_{k-1} + c_{k-1} I,\qquad c_k = -\frac{1}{k}\operatorname{tr}(J M_k),\qquad M_0 = 0,\ c_0 = 1
```

The Routh array of the coefficients then counts the roots in the right half plane: it is built row by row from the coefficients, and the number of sign changes down its first column equals the number of roots with positive real part. A whole row of zeros means roots on the imaginary axis, and the result is flagged `marginal`.

The code is in `pluss::ma::natsci::common::stability` (`stability::` below). `stability::characteristic_polynomial(J)` returns the coefficients {math}`c_0 \dots c_n`, `stability::routh(c)` returns `unstable`, `first_column` and `marginal`, `stability::unstable_modes(J)` does both steps, and `stability::local_stability(rate, x)` differentiates a rate function at the states `x` first. Every call accepts leading batch axes, `J` of shape `[..., n, n]`. The programs use the Brusselator, a model of an autocatalytic reaction, {math}`u' = 1 - (B+1)u + u^2v`, {math}`v' = Bu - u^2v`, whose equilibrium is {math}`(1, B)`.

## The characteristic polynomial and the Routh array

At {math}`B = 3` the Jacobian of the Brusselator at its equilibrium is {math}`[[2, 1], [-3, -1]]`. The program builds its characteristic polynomial, the first column of the Routh array and the count of unstable eigenvalues, and checks that `unstable_modes` agrees.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace stability = common::stability;
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // Brusselator with A = 1, B = 3 at its steady state (A, B/A): the Jacobian is [[B - 1, A^2], [-B, -A^2]]
  const auto jac = mat({{2.0, 1.0}, {-3.0, -1.0}});
  const auto poly = stability::characteristic_polynomial(jac);
  out.writeln("det(lambda I - J) coefficients c0..c2 = ", flat(poly));
  const auto r = stability::routh(poly);
  out.writeln("first column of the Routh array        = ", flat(r.first_column));
  out.writeln("eigenvalues with Re > 0                = ", flat(r.unstable), ", marginal = ", flat(r.marginal));
  const auto direct = stability::unstable_modes(jac);
  out.writeln("unstable_modes(J) gives the same count = ", flat(direct.unstable));
}
```

Output

```text
det(lambda I - J) coefficients c0..c2 = [1.0000, -1.0000, 1.0000]
first column of the Routh array        = [1.0000, -1.0000, 1.0000]
eigenvalues with Re > 0                = [2.0000], marginal = [0.0000]
unstable_modes(J) gives the same count = [2.0000]
```

The polynomial is {math}`\lambda^2 - \lambda + 1` (coefficients `1, -1, 1`). Its first Routh column `1, -1, 1` changes sign twice, so both eigenvalues have a positive real part: `2` unstable modes, `marginal = 0`. This is a spiral that grows, and `unstable_modes(J)` returns the same count in one call.

## Sweeping a parameter

To see how the equilibrium changes with {math}`B`, the program evaluates `local_stability` of the Brusselator rate at the equilibrium {math}`(1, B)` for six values of {math}`B`. `local_stability` builds the Jacobian by central differences of the rate you pass, so you do not write the derivatives.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace stability = common::stability;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the Brusselator  u' = A - (B + 1) u + u^2 v,  v' = B u - u^2 v  with A = 1; its steady state is (A, B / A)
static auto brusselator(double b) {
  return [=](const pluss::Series &x) {
    const auto u = x.select(-1, 0), v = x.select(-1, 1);
    const auto du = u.mul(-(b + 1.0)).add(1.0).add(u.square().mul(v));
    const auto dv = u.mul(b).sub(u.square().mul(v));
    return du.unsqueeze(-1).unite(std::vector<pluss::Series>{dv.unsqueeze(-1)}, -1);
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // sweep the parameter B: at each value, evaluate the stability of the steady state (1, B)
  for (const double b : {0.5, 1.5, 1.9, 2.1, 3.0, 5.0}) {
    const auto r = stability::local_stability(brusselator(b), vec({1.0, b}));
    out.writeln("B = ", b, "   unstable eigenvalues = ", flat(r.unstable), "   marginal = ", flat(r.marginal));
  }
}
```

Output

```text
B = 0.5000   unstable eigenvalues = [0.0000]   marginal = [0.0000]
B = 1.5000   unstable eigenvalues = [0.0000]   marginal = [0.0000]
B = 1.9000   unstable eigenvalues = [0.0000]   marginal = [0.0000]
B = 2.1000   unstable eigenvalues = [2.0000]   marginal = [0.0000]
B = 3.0000   unstable eigenvalues = [2.0000]   marginal = [0.0000]
B = 5.0000   unstable eigenvalues = [2.0000]   marginal = [0.0000]
```

The count is `0` for {math}`B = 0.5, 1.5` and `1.9` and `2` for {math}`B = 2.1, 3` and `5`. Both eigenvalues cross the imaginary axis together, between `1.9` and `2.1`: a Hopf point, where a steady state gives way to oscillations. Its exact place is {math}`B = 1 + A^2 = 2`; [the next page](continuation.md) finds it without a grid.

## A centre, a saddle and a batch

The program counts the unstable modes of three different cases: the centre of the predator-prey model, whose eigenvalues are {math}`\pm i`, the Lorenz system at the origin, and a batch of three Jacobians passed as one `[3, 2, 2]` array.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace stability = common::stability;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the predator-prey centre: the Jacobian at the coexistence point of Lotka-Volterra has eigenvalues +-i sqrt(a c)
  const auto centre = stability::unstable_modes(mat({{0.0, -1.0}, {1.0, 0.0}}));
  out.writeln("predator-prey centre:   unstable = ", flat(centre.unstable), "  marginal = ", flat(centre.marginal));
  // Lorenz at the origin with s = 10, r = 28, b = 8/3
  const double s = 10.0, r = 28.0, b = 8.0 / 3.0;
  const auto lorenz = stability::unstable_modes(mat({{-s, s, 0.0}, {r, -1.0, 0.0}, {0.0, 0.0, -b}}));
  out.writeln("Lorenz origin, r = 28:  unstable = ", flat(lorenz.unstable), "  marginal = ", flat(lorenz.marginal));
  // a whole batch of Jacobians in one call: [3, 2, 2]
  const auto batch = pluss::series::make(std::vector<double>{-1, 0, 0, -2,   1, 0, 0, 2,   0, 1, -1, 0}, {3, 2, 2}).to(pluss::ScalarType::Float64);
  const auto many = stability::unstable_modes(batch);
  out.writeln("sink, source, centre:   unstable = ", flat(many.unstable), "  marginal = ", flat(many.marginal));
}
```

Output

```text
predator-prey centre:   unstable = [0.0000]  marginal = [1.0000]
Lorenz origin, r = 28:  unstable = [1.0000]  marginal = [0.0000]
sink, source, centre:   unstable = [0.0000, 2.0000, 0.0000]  marginal = [0.0000, 0.0000, 1.0000]
```

The centre has `unstable = 0` and `marginal = 1`: its eigenvalues lie on the imaginary axis. The Lorenz origin at {math}`r = 28` has `1` unstable direction, a saddle. In the batch the sink, the source and the centre give `0, 2, 0` unstable modes and `0, 0, 1` marginal flags: three Jacobians, one call.

## Decisions

- Use `unstable_modes` when you already have a Jacobian and `local_stability` when you have only the rate function; the second differentiates for you and the step can be set with `relative_step`.
- Treat `marginal = 1` as "cannot tell by this test": a centre is not asymptotically stable, and a small change of the model decides between a spiral in and a spiral out.
- Use the method for models with a handful of variables. The characteristic polynomial is accurate for Jacobians up to about ten rows; for larger systems the count loses digits.
- To find the parameter at which the count changes, do not bisect on a grid by hand: continue the equilibrium through the parameter, as on the next page.

## Pitfalls

The count describes the Jacobian at the state you pass, so it means something only at an equilibrium. The program asks the same question at the equilibrium of the Brusselator and at a point that is not one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace stability = common::stability;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the Brusselator  u' = A - (B + 1) u + u^2 v,  v' = B u - u^2 v  with A = 1; its steady state is (A, B / A)
static auto brusselator(double b) {
  return [=](const pluss::Series &x) {
    const auto u = x.select(-1, 0), v = x.select(-1, 1);
    const auto du = u.mul(-(b + 1.0)).add(1.0).add(u.square().mul(v));
    const auto dv = u.mul(b).sub(u.square().mul(v));
    return du.unsqueeze(-1).unite(std::vector<pluss::Series>{dv.unsqueeze(-1)}, -1);
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // B = 3 has the steady state (1, 3). Asking about another point answers a different question: the count is the Jacobian's there
  const auto f = brusselator(3.0);
  const auto at_steady = stability::local_stability(f, vec({1.0, 3.0}));
  const auto elsewhere = stability::local_stability(f, vec({0.2, 0.2}));
  out.writeln("at the steady state (1, 3):   unstable = ", flat(at_steady.unstable), "   rate there = ", flat(f(vec({1.0, 3.0}))));
  out.writeln("at (0.2, 0.2):                unstable = ", flat(elsewhere.unstable), "   rate there = ", flat(f(vec({0.2, 0.2}))));
}
```

Output

```text
at the steady state (1, 3):   unstable = [2.0000]   rate there = [0.0000, 0.0000]
at (0.2, 0.2):                unstable = [0.0000]   rate there = [0.2080, 0.5920]
```

At the equilibrium {math}`(1, 3)` the rate is `0, 0` and the count is `2`. At {math}`(0.2, 0.2)` the rate is `0.2080, 0.5920`, the state is moving, and the count `0` describes the local geometry of a point that is not at rest. Check that the rate is near zero before reading the count as stability.

`unstable = 0` is not the same as stable. The program compares the centre with a damped spiral.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace stability = common::stability;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // an eigenvalue pair ON the imaginary axis is not "unstable", and not asymptotically stable either
  const auto centre = stability::unstable_modes(mat({{0.0, -1.0}, {1.0, 0.0}}));
  const auto damped = stability::unstable_modes(mat({{-0.1, -1.0}, {1.0, -0.1}}));
  out.writeln("centre:       unstable = ", flat(centre.unstable), "  marginal = ", flat(centre.marginal));
  out.writeln("damped spiral: unstable = ", flat(damped.unstable), "  marginal = ", flat(damped.marginal));
  const bool centre_attracts = flat(centre.unstable)[0] == 0.0 && flat(centre.marginal)[0] == 0.0;
  out.writeln("unstable == 0 alone says the centre attracts: ", centre_attracts ? "yes" : "no", " (marginal has to be 0 as well)");
}
```

Output

```text
centre:       unstable = [0.0000]  marginal = [1.0000]
damped spiral: unstable = [0.0000]  marginal = [0.0000]
unstable == 0 alone says the centre attracts: no (marginal has to be 0 as well)
```

The centre and the damped spiral both report `unstable = 0`, but the centre has `marginal = 1` and the spiral `marginal = 0`. Only the second attracts: asking the first question alone, "is the unstable count zero?", answers `yes` for the centre, which does not attract.

Matrices that are not Jacobians, or that contain a non-finite entry, are rejected with an error, never counted. The program passes a stoichiometry matrix, a Jacobian with a NaN and a polynomial whose leading coefficient is zero.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace stability = common::stability;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  // a stoichiometry matrix is not a Jacobian, and a NaN in the Jacobian must not read as "stable"
  try {
    (void)stability::characteristic_polynomial(mat({{1.0, -1.0, 0.0}, {0.0, 1.0, -1.0}}));
  } catch (const pluss::Error &e) {
    out.writeln("rectangular matrix: ", e.message());
  }
  try {
    (void)stability::unstable_modes(mat({{std::nan(""), 1.0}, {0.0, -1.0}}));
  } catch (const pluss::Error &e) {
    out.writeln("NaN entry:          ", e.message());
  }
  try {
    (void)stability::routh(vec({0.0, 1.0, 2.0}));
  } catch (const pluss::Error &e) {
    out.writeln("zero leading term:  ", e.message());
  }
}
```

Output

```text
rectangular matrix:  natsci: stability: Jacobian must be a (batch of) square matrices
NaN entry:           natsci: stability: Jacobian must be finite
zero leading term:   natsci: stability: leading coefficient must be non-zero
```

Each message names the rule that failed. A NaN in a Jacobian usually means the rate function was evaluated outside its domain, for instance at a negative concentration under a logarithm; the error points there instead of reporting a stable equilibrium.

## Where next

[The next page](continuation.md) follows an equilibrium through a parameter and finds the Hopf point of this page exactly. Models whose equilibria this method classifies appear in [Biology 2: Interacting Species](../../biology/bio_02_interactions.md) and [Biology 14: Gene Circuits](../../biology/bio_14_gene_circuits.md); delay models, whose stability is decided differently, are in [Biology 21: Delays](../../biology/bio_21_delays.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
