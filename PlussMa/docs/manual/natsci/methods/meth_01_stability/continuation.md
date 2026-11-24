# Following equilibria through a parameter

[Back to Methods 1: Stability and Branches](../meth_01_stability.md)

You have a model with one parameter and you want to know what happens to its steady states when that parameter changes: where they appear or vanish, where they lose stability, and where a new branch of states splits off. Sweeping the parameter on a grid and re-solving misses every fold, because a branch that turns back has no solution for a stretch of the parameter. This page follows a branch of equilibria around its turns and reports the bifurcation points it passes: folds, Hopf points and branch points.

## How it works

An equilibrium problem {math}`G(u, p) = 0` with {math}`u \in \mathbb{R}^n` and one parameter {math}`p` defines a curve in the {math}`(n+1)`-dimensional space of {math}`z = (u, p)`. Pseudo-arclength continuation walks along that curve rather than along {math}`p`: from a point it predicts the next one along the tangent {math}`t`, and corrects back onto the curve with Newton's method on the bordered system

```{math}
\begin{pmatrix} G(z) \\ t^{\top}(z - z_{\mathrm{pred}}) \end{pmatrix} = 0
```

The tangent solves {math}`[G_u\ G_p;\ t_{\mathrm{prev}}^{\top}]\,t = e_{n+1}`, so the curve can run backwards in {math}`p` and no turn is lost. Three kinds of special point are detected on the way and refined by bisection on the arclength:

- a **fold** (saddle-node), where the {math}`p`-component of the tangent changes sign;
- a **Hopf point**, where the number of eigenvalues of {math}`G_u` with positive real part changes by two while {math}`\det G_u` keeps its sign (the count of [the previous page](linear_stability.md), no eigensolver); `omega` is the frequency of the critical pair;
- a **branch point**, where the determinant of the bordered matrix changes sign: a transcritical or pitchfork point where another branch crosses.

The code is in `pluss::ma::natsci::common::continuation` (`continuation::` below). `continuation::continue_branch(G, u0, p0, options)` returns a `Branch` with `u` `[N, n]`, `p` `[N]`, the `unstable` count at every point, the list `special`, and the reason it stopped. The residual `G(u, p)` is called with `u` of shape `[K, n]` and `p` of shape `[K]`: every derivative is a finite difference, and all of its evaluation points go through `G` in one call, so `G` must treat the leading axis as independent lanes. `continuation::branch_directions(G, u, p, t)` finds the directions of the branches that leave a branch point.

## A Hopf point

The Brusselator again, now with {math}`B` as the continuation parameter. The program starts at the equilibrium {math}`(1, 0.5)` for {math}`B = 0.5`, follows the branch up to {math}`B = 4` and prints the special points with the unstable count at the two ends.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace continuation = common::continuation;
using pluss::Series;
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the residual G(u, p) of an equilibrium problem is called with u [K, n] and p [K]: K independent lanes, the last axis the species
static Series stack2(const Series &a, const Series &b) { return a.unsqueeze(-1).unite(std::vector<Series>{b.unsqueeze(-1)}, -1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // the Brusselator u' = 1 - (B + 1) u + u^2 v,  v' = B u - u^2 v, continued in B from the steady state (1, B)
  const continuation::Residual g = [](const Series &u, const Series &b) {
    const auto x = u.select(-1, 0), y = u.select(-1, 1);
    return stack2(x.mul(b.add(1.0)).neg().add(1.0).add(x.square().mul(y)), x.mul(b).sub(x.square().mul(y)));
  };
  continuation::Options opt;
  opt.p_min = 0.5;
  opt.p_max = 4.0;
  const auto branch = continuation::continue_branch(g, vec({1.0, 0.5}), 0.5, opt);
  out.writeln("points on the branch: ", branch.points, ", stopped: ", branch.stopped);
  out.writeln("B at the first and last point: ", flat(branch.p.select(0, 0)), " ", flat(branch.p.select(0, branch.points - 1)));
  out.writeln("unstable eigenvalues there:    ", flat(branch.unstable.select(0, 0)), " ", flat(branch.unstable.select(0, branch.points - 1)));
  for (const auto &s : branch.special)
    out.writeln(continuation::to_string(s.kind), " at B = ", s.p, ", u = ", s.u, ", frequency omega = ", s.omega);
}
```

Output

```text
points on the branch: 15, stopped: p_min/p_max reached
B at the first and last point: [0.50000] [4.24093]
unstable eigenvalues there:    [0.00000] [2.00000]
hopf at B = 2.00000, u = [1.00000, 2.00000], frequency omega = 1.00000
```

The branch has `15` points and stops at the parameter window. The unstable count goes from `0` at {math}`B = 0.5` to `2` at the end, and one Hopf point is reported at {math}`B = 2.00000`, {math}`u = (1, 2)`, with frequency {math}`\omega = 1.00000`. These are the exact values, {math}`B = 1 + A^2` and {math}`\omega = A` with {math}`A = 1`, found without a grid. The last point lies beyond `4` because the final step crosses the window.

## Folds and a window of two states

A population with logistic growth and a saturating loss, as in a spruce-budworm outbreak, {math}`u' = 0.56\,u(1 - u/K) - u^2/(1 + u^2)`, followed in the carrying capacity {math}`K`. The program prints the folds and the states at five points along the branch, with their unstable counts.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace continuation = common::continuation;
using pluss::Series;
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the residual G(u, p) of an equilibrium problem is called with u [K, n] and p [K]: K independent lanes, the last axis the species
static Series stack2(const Series &a, const Series &b) { return a.unsqueeze(-1).unite(std::vector<Series>{b.unsqueeze(-1)}, -1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // spruce-budworm outbreak:  u' = 0.56 u (1 - u / K) - u^2 / (1 + u^2),  continued in the carrying capacity K
  const continuation::Residual g = [](const Series &u, const Series &k) {
    return u.mul(0.56).mul(u.div(k.unsqueeze(-1)).neg().add(1.0)).sub(u.square().div(u.square().add(1.0)));
  };
  continuation::Options opt;
  opt.p_min = 1.0;
  opt.p_max = 20.0;
  const auto branch = continuation::continue_branch(g, vec({0.8}), 2.0, opt);
  out.writeln("points: ", branch.points, ", stopped: ", branch.stopped);
  for (const auto &s : branch.special)
    out.writeln(continuation::to_string(s.kind), " at K = ", s.p, ", u = ", s.u);
  // the folds bracket a window of bistability; the unstable count labels each piece of the branch
  const auto p = flat(branch.p), n = flat(branch.unstable);
  for (const std::int64_t i : {std::int64_t{0}, branch.points / 4, branch.points / 2, 3 * branch.points / 4, branch.points - 1})
    out.writeln("K = ", p[static_cast<std::size_t>(i)], "  u = ", flat(branch.u.select(0, i)), "  unstable = ", n[static_cast<std::size_t>(i)]);
}
```

Output

```text
points: 202, stopped: p_min/p_max reached
fold at K = 9.9344, u = [1.1391]
fold at K = 6.4457, u = [2.8166]
K = 2.0497  u = [0.5320]  unstable = 0.0000
K = 9.9338  u = [1.1442]  unstable = 1.0000
K = 7.4466  u = [1.7978]  unstable = 1.0000
K = 8.9766  u = [6.6037]  unstable = 0.0000
K = 20.1664  u = [18.1929]  unstable = 0.0000
```

The branch has `202` points and two folds, at {math}`K = 9.9344` and {math}`K = 6.4457`. Between them the branch is S-shaped and there are three equilibria for one {math}`K`: the middle piece, with `unstable = 1`, is the threshold, and the outer pieces (`0` unstable, at {math}`K = 2.0497` and above {math}`K = 8.9766`) are the low and high states. A population that has risen to the high state at {math}`K = 9` does not fall back until {math}`K` drops below `6.4457`. A grid on {math}`K` would have found one solution or three but not the turns.

## A branch point

A toggle switch is two genes that repress each other: {math}`x' = a/(1+y^2) - x`, {math}`y' = a/(1+x^2) - y`, with the expression rate {math}`a` as the parameter. The program follows the symmetric state {math}`x = y` from small {math}`a` and asks for the directions of the branches at the branch point it finds.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace continuation = common::continuation;
using pluss::Series;
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the residual G(u, p) of an equilibrium problem is called with u [K, n] and p [K]: K independent lanes, the last axis the species
static Series stack2(const Series &a, const Series &b) { return a.unsqueeze(-1).unite(std::vector<Series>{b.unsqueeze(-1)}, -1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a toggle switch: two genes repress each other.  x' = a / (1 + y^2) - x,  y' = a / (1 + x^2) - y, continued in the expression rate a
  const continuation::Residual g = [](const Series &u, const Series &a) {
    const auto x = u.select(-1, 0), y = u.select(-1, 1);
    return stack2(a.div(y.square().add(1.0)).sub(x), a.div(x.square().add(1.0)).sub(y));
  };
  continuation::Options opt;
  opt.p_min = 0.1;
  opt.p_max = 6.0;
  const auto symmetric = continuation::continue_branch(g, vec({0.1, 0.1}), 0.1, opt);
  const auto &bp = symmetric.special.front();
  out.writeln("symmetric branch: unstable modes at a = 0.1: ", flat(symmetric.unstable.select(0, 0)), ", at the end (a = ", flat(symmetric.p.select(0, symmetric.points - 1)), "): ", flat(symmetric.unstable.select(0, symmetric.points - 1)));
  out.writeln(continuation::to_string(bp.kind), " at a = ", bp.p, ", u = ", bp.u);
  // at a branch point the tangent of the branch we came along is known: here (x, y, a) moves along the symmetric line
  const auto dirs = continuation::branch_directions(g, vec(bp.u), bp.p, {0.7, 0.7, 0.1});
  out.writeln("directions found: ", dirs.size());
  for (const auto &d : dirs)
    out.writeln("  direction (dx, dy, da) = ", d);
}
```

Output

```text
symmetric branch: unstable modes at a = 0.1: [0.0000], at the end (a = [6.0770]): [1.0000]
branch point at a = 2.0000, u = [1.0000, 1.0000]
directions found: 2
  direction (dx, dy, da) = [-0.7071, 0.7071, 0.0000]
  direction (dx, dy, da) = [0.7035, 0.7035, 0.1005]
```

The symmetric branch has `0` unstable modes at {math}`a = 0.1` and `1` at the end of the run: it loses stability on the way, at the first special point, a branch point at {math}`a = 2.0000`, {math}`u = (1, 1)`. `branch_directions` returns `2` directions in {math}`(dx, dy, da)`: `(0.7035, 0.7035, 0.1005)` continues along the symmetric line, and `(-0.7071, 0.7071, 0.0000)` leaves it at constant {math}`a`, with {math}`x` and {math}`y` moving in opposite directions.

## Switching branches

The program starts a little way from the branch point along the new direction, and along its opposite, and follows both with `continue_branch`, passing the direction as the starting tangent.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace continuation = common::continuation;
using pluss::Series;
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the residual G(u, p) of an equilibrium problem is called with u [K, n] and p [K]: K independent lanes, the last axis the species
static Series stack2(const Series &a, const Series &b) { return a.unsqueeze(-1).unite(std::vector<Series>{b.unsqueeze(-1)}, -1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const continuation::Residual g = [](const Series &u, const Series &a) {
    const auto x = u.select(-1, 0), y = u.select(-1, 1);
    return stack2(a.div(y.square().add(1.0)).sub(x), a.div(x.square().add(1.0)).sub(y));
  };
  continuation::Options opt;
  opt.p_min = 0.1;
  opt.p_max = 6.0;
  const auto symmetric = continuation::continue_branch(g, vec({0.1, 0.1}), 0.1, opt);
  const auto &bp = symmetric.special.front();
  const auto dirs = continuation::branch_directions(g, vec(bp.u), bp.p, {0.7, 0.7, 0.1});
  // the second direction is the symmetric line itself; the one that changes x - y leaves it. A branch and its mirror image (x and y swapped)
  // are the two signs of that direction: start a little way along +d and along -d, with the matching tangent
  for (const auto &d : dirs) {
    if (std::abs(d[0] - d[1]) < 1e-3)
      continue;
    for (const double sign : {1.0, -1.0}) {
      const std::vector<double> t{sign * d[0], sign * d[1], sign * d[2]};
      const auto branch = continuation::continue_branch(g, vec({bp.u[0] + 1e-4 * t[0], bp.u[1] + 1e-4 * t[1]}), bp.p + 1e-4 * t[2], opt, 1.0, t);
      const auto last = branch.points - 1;
      out.writeln("start direction (dx, dy) = ", std::vector<double>{t[0], t[1]}, ": ", branch.points, " points, stopped: ", branch.stopped);
      out.writeln("  at the end a = ", flat(branch.p.select(0, last)), ", (x, y) = ", flat(branch.u.select(0, last)), ", unstable = ", flat(branch.unstable.select(0, last)));
    }
  }
}
```

Output

```text
start direction (dx, dy) = [-0.7071, 0.7071]: 118 points, stopped: p_min/p_max reached
  at the end a = [6.0542], (x, y) = [0.1699, 5.8843], unstable = [0.0000]
start direction (dx, dy) = [0.7071, -0.7071]: 118 points, stopped: p_min/p_max reached
  at the end a = [6.0542], (x, y) = [5.8843, 0.1699], unstable = [0.0000]
```

The two branches are mirror images: at {math}`a = 6.0542` one has {math}`(x, y) = (0.1699, 5.8843)` and the other {math}`(5.8843, 0.1699)`. Gene {math}`y` wins in one and gene {math}`x` in the other, and both have `0` unstable modes: the symmetric state became unstable at {math}`a = 2` and the cell chooses one of two stable states. This is the bistable switch of [Biology 14: Gene Circuits](../../biology/bio_14_gene_circuits.md) found from the equations.

## Decisions

- Choose the parameter window with `p_min` and `p_max`: it ends the run. The default window is unbounded, and the run then stops only at `max_points`.
- Start from a point that is an equilibrium (the corrector polishes the start, but a start far from the branch fails with an error) and pick the direction with the last argument, `1.0` or `-1.0`.
- Set `stability = false` only if `G` is not a vector field, for example a transformed residual; the unstable counts and the Hopf test are then switched off.
- If a branch is thin or has features close together, lower `step_max`; if it is long and smooth, raise it. `u_scale` weights the components in the arclength when they differ by orders of magnitude.

## Pitfalls

A run that ends because of the point budget, not the end of the branch, still returns a branch, and it is shorter than it looks. The program follows the budworm branch with `max_points` of `60` and of `2000`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace continuation = common::continuation;
using pluss::Series;
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the residual G(u, p) of an equilibrium problem is called with u [K, n] and p [K]: K independent lanes, the last axis the species
static Series stack2(const Series &a, const Series &b) { return a.unsqueeze(-1).unite(std::vector<Series>{b.unsqueeze(-1)}, -1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const continuation::Residual g = [](const Series &u, const Series &k) {
    return u.mul(0.56).mul(u.div(k.unsqueeze(-1)).neg().add(1.0)).sub(u.square().div(u.square().add(1.0)));
  };
  // a point budget that is too small ends the run before the branch has shown its folds: read branch.stopped
  continuation::Options small;
  small.p_min = 1.0;
  small.p_max = 20.0;
  small.max_points = 60;
  const auto cut = continuation::continue_branch(g, vec({0.8}), 2.0, small);
  out.writeln("max_points = 60:   ", cut.points, " points, stopped: ", cut.stopped, ", special points found: ", cut.special.size());
  small.max_points = 2000;
  const auto full = continuation::continue_branch(g, vec({0.8}), 2.0, small);
  out.writeln("max_points = 2000: ", full.points, " points, stopped: ", full.stopped, ", special points found: ", full.special.size());
}
```

Output

```text
max_points = 60:   60 points, stopped: max_points, special points found: 1
max_points = 2000: 202 points, stopped: p_min/p_max reached, special points found: 2
```

With `60` points the run stops with `stopped: max_points` and has found `1` special point of the `2` that exist. With `2000` it stops at the parameter window, with `202` points and both folds. Always read `branch.stopped` before concluding that a bifurcation is absent.

The unstable counts and the Hopf test read the residual as the right-hand side of {math}`u' = G(u, p)`. The program writes the Brusselator residual twice, as "production minus consumption" and as the opposite.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace continuation = common::continuation;
using pluss::Series;
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the residual G(u, p) of an equilibrium problem is called with u [K, n] and p [K]: K independent lanes, the last axis the species
static Series stack2(const Series &a, const Series &b) { return a.unsqueeze(-1).unite(std::vector<Series>{b.unsqueeze(-1)}, -1); }

static Series brusselator(const Series &u, const Series &b, double sign) {
  const auto x = u.select(-1, 0), y = u.select(-1, 1);
  return stack2(x.mul(b.add(1.0)).neg().add(1.0).add(x.square().mul(y)).mul(sign), x.mul(b).sub(x.square().mul(y)).mul(sign));
}
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // the unstable count and the Hopf test read the residual as the vector field of u' = G(u, p). Written as "consumption minus production" the sign is
  // reversed: the equilibria are the same but every eigenvalue changes sign, so the labels flip
  continuation::Options opt;
  opt.p_min = 0.5;
  opt.p_max = 4.0;
  for (const double sign : {1.0, -1.0}) {
    const auto b = continuation::continue_branch([sign](const Series &u, const Series &p) { return brusselator(u, p, sign); }, vec({1.0, 0.5}), 0.5, opt);
    out.writeln(sign > 0 ? "production - consumption:" : "consumption - production:", "  unstable at B = 0.5: ", flat(b.unstable.select(0, 0)), ", at the end: ", flat(b.unstable.select(0, b.points - 1)), ", Hopf points found: ", b.special.size());
  }
}
```

Output

```text
production - consumption:  unstable at B = 0.5: [0.00000], at the end: [2.00000], Hopf points found: 1
consumption - production:  unstable at B = 0.5: [2.00000], at the end: [0.00000], Hopf points found: 1
```

Both runs find the same Hopf point, since the equilibria are the same, but the unstable count is `0` at {math}`B = 0.5` and `2` at the end for the first, and `2` and `0` for the second: every eigenvalue changed sign, so the labels are reversed. Write the residual in the direction of time.

A start that is not a point on a branch, a start that is not a vector, or a start outside the window.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace continuation = common::continuation;
using pluss::Series;
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the residual G(u, p) of an equilibrium problem is called with u [K, n] and p [K]: K independent lanes, the last axis the species
static Series stack2(const Series &a, const Series &b) { return a.unsqueeze(-1).unite(std::vector<Series>{b.unsqueeze(-1)}, -1); }

int main() {
  OutputEngine out;
  const continuation::Residual g = [](const Series &u, const Series &k) {
    return u.mul(0.56).mul(u.div(k.unsqueeze(-1)).neg().add(1.0)).sub(u.square().div(u.square().add(1.0)));
  };
  continuation::Options opt;
  try {
    (void)continuation::continue_branch(g, vec({std::nan("")}), 2.0, opt);
  } catch (const pluss::Error &e) {
    out.writeln("NaN start:        ", e.message());
  }
  try {
    (void)continuation::continue_branch(g, pluss::series::make(0.8).to(pluss::ScalarType::Float64), 2.0, opt);
  } catch (const pluss::Error &e) {
    out.writeln("scalar start:     ", e.message());
  }
  opt.p_min = 5.0;
  opt.p_max = 20.0;
  try {
    const auto b = continuation::continue_branch(g, vec({0.8}), 2.0, opt);
    out.writeln("start outside the window: ", b.points, " points, stopped: ", b.stopped);
  } catch (const pluss::Error &e) {
    out.writeln("start outside the window: ", e.message());
  }
}
```

Output

```text
NaN start:         natsci: continuation: the starting point does not converge to a solution branch
scalar start:      natsci: continuation: u0 must be a vector [n]
start outside the window:  2  points, stopped:  p_min/p_max reached
```

A NaN start does not converge onto a branch and is rejected, and a scalar start is rejected with the shape that is expected. A start outside the window is not an error: the branch has `2` points and `stopped: p_min/p_max reached`, which is easy to mistake for a result.

## Where next

[Linear stability](linear_stability.md) is the count used for the unstable column. The branches of this page are the steady states of models in [Biology 2: Interacting Species](../../biology/bio_02_interactions.md) and [Biology 14: Gene Circuits](../../biology/bio_14_gene_circuits.md); [Methods 2: Fitting and Identifiability](../meth_02_fitting.md) asks how well the parameters of such a model can be recovered from data.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="linear_stability.html">
    <span class="label">Previous</span>
    <span class="title">Linear stability of an equilibrium</span>
  </a>
</div>
<!-- apiref:nav:end -->
