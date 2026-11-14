<!-- generated from static_analysis.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Static analysis

[Back to Structures 1: Trusses](../structure_01_trusses.md)

A truss under steady loads settles into an equilibrium: every joint moves a little, every bar stretches or shortens, and the supports push back. `structure::solve` finds that equilibrium. This page solves a plane truss and checks every number against a hand calculation, repeats the exercise for a space truss, shows a truss whose bar forces depend on the bar stiffnesses, and explains the diagnostics that tell you the solve can be trusted.

## The mental model

`solve(truss, load, fix)` takes the compiled `Truss`, a `load` array with one force vector per node (shape `[..., node, axis]`) and a `Fix` naming the supported degrees of freedom. It returns a `StaticResult`:

| Member | Shape (single structure) | Meaning |
|---|---|---|
| `displacement` | `[node, axis]` | how far each node moves |
| `reaction` | `[node, axis]` | the force each support applies; zero at free degrees of freedom |
| `force` | `[bar]` | axial force in each bar, positive in tension |
| `stress` | `[bar]` | `force / area` |
| `strain` | `[bar]` | `stress / modulus`, the relative change in length |
| `energy` | `[]` | total strain energy stored in the bars |
| `residual_norm` | `[]` | size of the unbalanced force left in the solve |
| `converged` | `[]` | one if the iterative solve reached its tolerance |
| `breakdown` | `[]` | one if the solve found the structure to be a mechanism |
| `iterations` | (a number) | iterations used by the solve |

The solution satisfies `K u = f`, where `K` is the stiffness of the whole truss assembled from the bar stiffnesses `E A / L` and the bar directions. The solver never forms `K` as a matrix: it applies it bar by bar inside a preconditioned conjugate-gradient iteration, which is why large trusses and many load cases cost little.

## A two-bar truss, checked by hand

The truss of the previous page again: feet at `(0, 0)` and `(6, 0)`, apex at `(3, 4)`, bars of `L = 5 m`, area `1e-3 m^2` and modulus `200 GPa`. A 10 kN load pushes the apex down; both feet are pinned (fixed in both directions).

Equilibrium of the apex fixes the bar forces: each bar makes an angle with the horizontal whose sine is `4/5`, so the two vertical components carry the load, `2 F sin(theta) = 10 kN` in compression, and `F = -10 kN / (2 * 0.8) = -6.25 kN`. The apex drops by `delta = sum(F f L / (E A))`, where `f = -0.625` is the force per unit of load, so `delta = 2 * 6250 * 0.625 * 5 / 2e8 = 0.1953 mm`. The strain energy equals half the work of the load, `0.5 * 10000 * delta`. The program prints the solver's value beside each of these.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double area = 1e-3, modulus = 200e9, load = 10000.0;
  const auto truss = st::truss({grid({0, 0, 6, 0, 3, 4}, 2), {0, 1}, {2, 2}}, {vec({area, area}), vec({modulus, modulus})});
  const st::Fix feet{{0, 0, 1, 1}, {0, 1, 0, 1}, {}};
  const auto r = st::solve(truss, grid({0, 0, 0, 0, 0, -load}, 2), feet);

  const double force = -load / (2 * 0.8);
  const double drop = 2 * (-force) * 0.625 * 5.0 / (modulus * area);
  out.writeln("bar force (N)        solver ", flat(r.force), "   hand ", force);
  out.writeln("bar stress (MPa)     solver ", flat(r.stress.mul(1e-6)), "   hand ", force / area * 1e-6);
  out.writeln("bar strain (1e-6)    solver ", flat(r.strain.mul(1e6)), "   hand ", force / (area * modulus) * 1e6);
  out.writeln("apex drop (mm)       solver ", -1000.0 * flat(r.displacement)[5], "   hand ", 1000.0 * drop);
  out.writeln("apex sideways (mm)   solver ", 1000.0 * flat(r.displacement)[4], "   hand ", 0.0);
  out.writeln("strain energy (J)    solver ", r.energy.item<double>(), "   hand ", 0.5 * load * drop);
  out.writeln("reactions (N)        ", flat(r.reaction));
}
```

Output

```text
bar force (N)        solver [-6250.0000, -6250.0000]   hand -6250.0000
bar stress (MPa)     solver [-6.2500, -6.2500]   hand -6.2500
bar strain (1e-6)    solver [-31.2500, -31.2500]   hand -31.2500
apex drop (mm)       solver 0.1953   hand 0.1953
apex sideways (mm)   solver 0.0000   hand 0.0000
strain energy (J)    solver 0.9766   hand 0.9766
reactions (N)        [3750.0000, 5000.0000, -3750.0000, 5000.0000, 0.0000, 0.0000]
```

Every solver value equals its hand value. The reactions are the forces the supports apply: the left foot pushes the structure inward with `3750 N` and up with `5000 N`, and the right foot pushes inward (`-3750 N`) and up with `5000 N`, mirror images of each other. A strain of about `-31 microstrain` and a stress of `-6.25 MPa` mean the bars are in compression and far below any steel limit.

## Checking equilibrium from the result

Whatever the structure, a correct solution satisfies equilibrium at every joint: the bar forces, the support reactions and the applied load at a joint add to zero, and the reactions together balance the loads. The program tests both without any knowledge of the answer. At the apex the two bars, each pulling or pushing along its own direction, must cancel the 10 kN load; and the vertical and horizontal sums of the reactions must equal minus the corresponding sums of the loads.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto truss = st::truss({grid({0, 0, 6, 0, 3, 4}, 2), {0, 1}, {2, 2}}, {vec({1e-3, 1e-3}), vec({200e9, 200e9})});
  const st::Fix feet{{0, 0, 1, 1}, {0, 1, 0, 1}, {}};
  const std::vector<double> load = {0, 0, 0, 0, 0, -10000};
  const auto r = st::solve(truss, grid(load, 2), feet);

  const auto force = flat(r.force);
  const auto reaction = flat(r.reaction);
  // each bar pulls the apex toward the foot when it is in tension: along (foot - apex) / length
  const double pull_x = force[0] * (0.0 - 3.0) / 5.0 + force[1] * (6.0 - 3.0) / 5.0;
  const double pull_y = force[0] * (0.0 - 4.0) / 5.0 + force[1] * (0.0 - 4.0) / 5.0;
  out.writeln("apex balance, x: ", pull_x + load[4], "   y: ", pull_y + load[5]);

  double sum_x = 0, sum_y = 0, load_x = 0, load_y = 0;
  for (int node = 0; node < 3; ++node) {
    sum_x += reaction[2 * node];
    sum_y += reaction[2 * node + 1];
    load_x += load[2 * node];
    load_y += load[2 * node + 1];
  }
  out.writeln("reactions plus loads, x: ", sum_x + load_x, "   y: ", sum_y + load_y);
  out.writeln("reaction at the free apex: ", reaction[4], " ", reaction[5]);
}
```

Output

```text
apex balance, x: 0.000000   y: 0.000000
reactions plus loads, x: 0.000000   y: 0.000000
reaction at the free apex: 0.000000 0.000000
```

The balances are zero to the printed digits, and the apex, which is not supported, reports no reaction. Use these two checks on any structure you build.

## A space truss

The same machinery solves a truss in three dimensions with no change except a third coordinate. A tripod of three legs, each `2.5 m` long and inclined so that its direction cosine with the vertical is `2 / 2.5 = 0.8`, carries a 12 kN downward load at the apex. By symmetry the three legs carry equal forces, and the three vertical components carry the load: `3 F * 0.8 = -12 kN`, so `F = -5 kN`. The apex drops by `3 * F f L / (E A)` with `f = -5000 / 12000`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double area = 1e-3, modulus = 200e9, load = 12000.0;
  const auto tripod = st::truss({grid({1.5, 0, 0, -0.75, 1.299038105676658, 0, -0.75, -1.299038105676658, 0, 0, 0, 2}, 3), {0, 1, 2}, {3, 3, 3}},
                                {vec({area, area, area}), vec({modulus, modulus, modulus})});
  // all three degrees of freedom of the three feet are fixed
  const st::Fix feet{{0, 0, 0, 1, 1, 1, 2, 2, 2}, {0, 1, 2, 0, 1, 2, 0, 1, 2}, {}};
  const auto r = st::solve(tripod, grid({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -load}, 3), feet);

  const double force = -load / (3 * 0.8);
  const double drop = 3 * force * (force / load) * 2.5 / (modulus * area);
  const auto u = flat(r.displacement);
  out.writeln("leg forces (N)   solver ", flat(r.force), "   hand ", force);
  out.writeln("apex drop (mm)   solver ", -1000.0 * u[11], "   hand ", 1000.0 * drop);
  out.writeln("apex sideways (mm)       ", 1000.0 * u[9], " ", 1000.0 * u[10]);
  out.writeln("axes in the displacement array: ", r.displacement.sizes());
}
```

Output

```text
leg forces (N)   solver [-5000.0000, -5000.0000, -5000.0000]   hand -5000.0000
apex drop (mm)   solver 0.0781   hand 0.0781
apex sideways (mm)       0.0000 0.0000
axes in the displacement array: [4, 3]
```

Results carry the same number of axes as the structure: `displacement` and `reaction` are `[node, 3]`. The apex moves straight down, with no sideways motion by symmetry.

## When the forces depend on stiffness

The two-bar truss is **statically determinate**: its forces follow from equilibrium alone. A truss with more bars than equilibrium can fix is **statically indeterminate**, and its bar forces depend on how stiff each bar is. Take a load hung from three bars meeting at one joint: a vertical bar `4 m` long, and two inclined bars `5 m` long at the sides, all of equal area and modulus, with a 10 kN load at the joint.

Equilibrium gives one equation, `F_m + 2 * 0.8 F_s = P`. Compatibility, the requirement that the three bars meet at the same moved joint, gives a second. The joint drops by `delta`, the vertical bar stretches by `delta`, and each inclined bar by `0.8 delta`. Dividing by the lengths, `F_m = E A delta / 4` and `F_s = E A * 0.8 delta / 5`, so `F_s = 0.64 F_m` and `F_m (1 + 1.6 * 0.64) = P`: `F_m = P / 2.024 = 4940.7 N` and `F_s = 3162.1 N`. The middle bar, the stiffest path, carries the most.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // supports at the top (-3, 4), (0, 4), (3, 4); the loaded joint hangs at (0, 0)
  const st::Mesh mesh{grid({-3, 4, 0, 4, 3, 4, 0, 0}, 2), {0, 1, 2}, {3, 3, 3}};
  const st::Fix supports{{0, 0, 1, 1, 2, 2}, {0, 1, 0, 1, 0, 1}, {}};
  const double load = 10000.0;
  const auto load_array = grid({0, 0, 0, 0, 0, 0, 0, -load}, 2);

  const auto equal = st::truss(mesh, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
  const auto r = st::solve(equal, load_array, supports);
  out.writeln("equal bars:   side, middle, side forces ", flat(r.force));
  out.writeln("hand:         middle ", load / 2.024, "   side ", 0.64 * load / 2.024);

  // the middle bar made three times thicker takes a larger share
  const auto thick = st::truss(mesh, {vec({1e-3, 3e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
  const auto t = st::solve(thick, load_array, supports);
  out.writeln("thick middle: side, middle, side forces ", flat(t.force));
  out.writeln("vertical balance: ", flat(t.force)[1] + 2 * 0.8 * flat(t.force)[0] - load);
}
```

Output

```text
equal bars:   side, middle, side forces [3162.055, 4940.711, 3162.055]
hand:         middle 4940.711   side 3162.055
thick middle: side, middle, side forces [1590.457, 7455.268, 1590.457]
vertical balance: 0.000
```

With three times the area the middle bar takes `7455.3 N` of the `10 kN` (against `4940.7 N` before) and each side bar `1590.5 N`; the vertical balance `F_m + 1.6 F_s - P` is zero in both cases. In an indeterminate structure the sizing of one member changes the forces in all the others, so the structure must be re-solved after every change of a section.

## Reading the solver's diagnostics

The static solve is iterative, and the result says how it went. `converged` is one when the residual reached the tolerance, `breakdown` is one when the iteration met a direction of zero stiffness (a mechanism, see the [next page](supports_loads_and_sweeps.md)), `residual_norm` is the remaining unbalanced force and `iterations` is the iteration count. `SolveOptions` holds the conjugate-gradient controls: `cg.relative_tolerance`, `cg.absolute_tolerance` and `cg.max_iterations`, and `fail_on_nonconvergence`, which makes a failed solve throw (the default) or return its diagnostics.

Iterative solves cost more as a structure grows and gets more slender. The program builds a cantilever ladder truss of 40 bays, each 1 m wide and 1 m high with chords, verticals and one diagonal per bay (161 bars), fixed at its left end and loaded at the free bottom corner. It solves it at four tolerances and reports the work each takes and the tip deflection each gives, measured against the tightest solve.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));

  const int bays = 40;
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int level = 0; level < 2; ++level)
    for (int i = 0; i <= bays; ++i) {
      position.push_back(i);
      position.push_back(level);
    }
  const auto bottom = [&](int i) { return i; };
  const auto top = [&](int i) { return bays + 1 + i; };
  for (int i = 0; i < bays; ++i) {
    from.push_back(bottom(i)); to.push_back(bottom(i + 1));   // bottom chord
    from.push_back(top(i));    to.push_back(top(i + 1));      // top chord
    from.push_back(bottom(i)); to.push_back(top(i + 1));      // diagonal
  }
  for (int i = 0; i <= bays; ++i) { from.push_back(bottom(i)); to.push_back(top(i)); }   // verticals

  const auto bars = static_cast<std::int64_t>(from.size());
  const auto ladder = st::truss({grid(position, 2), from, to}, {vec(std::vector<double>(bars, 1e-3)), vec(std::vector<double>(bars, 200e9))});
  std::vector<double> load(4 * (bays + 1), 0.0);
  load[2 * bottom(bays) + 1] = -1000.0;   // 1 kN down at the free bottom corner
  const st::Fix wall{{bottom(0), bottom(0), top(0), top(0)}, {0, 1, 0, 1}, {}};

  out.writeln("bars ", ladder.bars(), ", nodes ", ladder.nodes());
  const auto tip = [&](const st::StaticResult &r) { return r.displacement.select(-2, bottom(bays)).select(-1, 1).item<double>(); };

  st::SolveOptions tightest;
  tightest.cg.relative_tolerance = 1e-14;
  tightest.cg.absolute_tolerance = 1e-30;
  const double reference = tip(st::solve(ladder, grid(load, 2), wall, tightest));
  out.writeln("tip deflection (m), tightest solve: ", reference);

  for (double tolerance : {1e-10, 1e-6, 1e-3}) {
    st::SolveOptions options;
    options.cg.relative_tolerance = tolerance;
    options.cg.absolute_tolerance = 1e-30;
    const auto r = st::solve(ladder, grid(load, 2), wall, options);
    out.writeln("relative tolerance ", tolerance, ": iterations ", r.iterations, ", converged ", r.converged.item<bool>(), ", residual ", r.residual_norm.item<double>(),
                ", tip error ", std::abs(tip(r) - reference));
  }
}
```

Output

```text
bars 161, nodes 82
tip deflection (m), tightest solve: -2.14e-01
relative tolerance 1.00e-10: iterations 240, converged true, residual 5.51e-08, tip error 2.16e-15
relative tolerance 1.00e-06: iterations 184, converged true, residual 7.86e-04, tip error 3.97e-13
relative tolerance 1.00e-03: iterations 176, converged true, residual 9.74e-01, tip error 1.00e-10
```

The structure behaves like a deep beam, and its deflection agrees with the beam formula: the chords of area `1e-3` sit `0.5 m` either side of the middle, so the second moment of area is `2 * 1e-3 * 0.5^2 = 5e-4 m^4` and `P L^3 / (3 E I) = 1000 * 40^3 / (3 * 200e9 * 5e-4) = 0.2133 m`, against the truss's `0.2142 m` (the difference is shear in the diagonals). A long structure needs a couple of hundred iterations, and each decade of tolerance saves a modest number. The default `1e-10` gives a tip deflection accurate to about `1e-15` metres here, far below anything the input data justify, and a loose `1e-3` still gives about nine digits of displacement while leaving a larger residual force. Change the tolerance only when the solve is a bottleneck, and check `converged` when you do.

If the iteration budget `cg.max_iterations` runs out before the tolerance is met, the solve throws an error that says so and names the budget; lower it deliberately and the error appears. A structure that is a mechanism is reported differently, as the [next page](supports_loads_and_sweeps.md) shows.

## The remaining conjugate-gradient controls

`SolveOptions::cg` has three more fields. `absolute_tolerance` is the residual, in newtons, below which the iteration stops whatever the load; `check_interval` is how often the convergence test is evaluated, with larger values spending iterations without testing; and `curvature_tolerance` is the threshold below which a search direction is treated as having zero stiffness, which is how a mechanism is recognised. The program solves the hung three-bar truss with the defaults, with the absolute tolerance on its own, with a test at every iteration, and with a curvature threshold so large that every direction counts as a mechanism.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const auto truss = st::truss({grid({-3, 4, 0, 4, 3, 4, 0, 0}, 2), {0, 1, 2}, {3, 3, 3}}, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
  const st::Fix supports{{0, 0, 1, 1, 2, 2}, {0, 1, 0, 1, 0, 1}, {}};
  const auto load = grid({0, 0, 0, 0, 0, 0, 0, -10000}, 2);
  const auto run = [&](const char *label, const st::SolveOptions &options) {
    const auto r = st::solve(truss, load, supports, options);
    out.writeln(label, ": iterations ", r.iterations, ", residual ", r.residual_norm.item<double>(), ", converged ", r.converged.item<bool>(), ", breakdown ", r.breakdown.item<bool>());
  };
  st::SolveOptions defaults;
  run("defaults                     ", defaults);
  st::SolveOptions absolute = defaults;
  absolute.cg.relative_tolerance = 0.0;
  absolute.cg.absolute_tolerance = 1e-3;
  run("absolute tolerance 1e-3 only ", absolute);
  st::SolveOptions every = defaults;
  every.cg.check_interval = 1;
  run("check_interval 1             ", every);
  st::SolveOptions curvature = defaults;
  curvature.cg.curvature_tolerance = 1e30;
  curvature.fail_on_nonconvergence = false;
  run("curvature_tolerance 1e30     ", curvature);
}
```

Output

```text
defaults                     : iterations 8, residual 0.00e+00, converged true, breakdown false
absolute tolerance 1e-3 only : iterations 8, residual 0.00e+00, converged true, breakdown false
check_interval 1             : iterations 1, residual 0.00e+00, converged true, breakdown false
curvature_tolerance 1e30     : iterations 0, residual 1.00e+04, converged false, breakdown true
```

The absolute tolerance on its own gives the same answer here, since the residual reaches it in the same eight iterations. Testing at every iteration notices that the solve has finished after one, where the default testing every eighth iteration runs eight: the answer is identical, and the count differs only in when the test noticed. A curvature threshold above every genuine stiffness makes the solve report a mechanism, `breakdown` true, on a structure that is perfectly sound, which is what the field is for: leave it at zero unless you want to flag directions that are nearly free.

## Decisions

- **A truss under steady loads:** `solve`, then read `force` for design, `displacement` for stiffness checks, `reaction` for the foundations and `energy` when comparing alternatives.
- **Checking an answer you do not know:** the two equilibrium checks above, a joint balance and a support balance, need no answer to compare with.
- **An indeterminate structure:** every section matters to every force, so re-solve after each change; do not reuse forces from an earlier design.

## Pitfalls

Two mistakes give numbers that look like an answer. The program makes both: it reads a bar's sign the wrong way round on the hung three-bar truss pushed upward, and it switches `fail_on_nonconvergence` off on the 40-bay ladder truss with an iteration budget of twenty, far too small for it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));

  // 1. the hung three-bar truss with the load reversed: 10 kN pushing the hung joint up
  const st::Mesh mesh{grid({-3, 4, 0, 4, 3, 4, 0, 0}, 2), {0, 1, 2}, {3, 3, 3}};
  const st::Fix supports{{0, 0, 1, 1, 2, 2}, {0, 1, 0, 1, 0, 1}, {}};
  const auto hung = st::truss(mesh, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
  const auto up = st::solve(hung, grid({0, 0, 0, 0, 0, 0, 0, 10000}, 2), supports);
  out.writeln("load pushing up: bar forces (N) ", flat(up.force), ", stresses (MPa) ", flat(up.stress.mul(1e-6)));

  // 2. the 40-bay ladder truss of the previous program, solved with a budget of 20 iterations
  const int bays = 40;
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int level = 0; level < 2; ++level)
    for (int i = 0; i <= bays; ++i) {
      position.push_back(i);
      position.push_back(level);
    }
  const auto bottom = [&](int i) { return i; };
  const auto top = [&](int i) { return bays + 1 + i; };
  for (int i = 0; i < bays; ++i) {
    from.push_back(bottom(i)); to.push_back(bottom(i + 1));
    from.push_back(top(i));    to.push_back(top(i + 1));
    from.push_back(bottom(i)); to.push_back(top(i + 1));
  }
  for (int i = 0; i <= bays; ++i) { from.push_back(bottom(i)); to.push_back(top(i)); }
  const auto bars = static_cast<std::int64_t>(from.size());
  const auto ladder = st::truss({grid(position, 2), from, to}, {vec(std::vector<double>(bars, 1e-3)), vec(std::vector<double>(bars, 200e9))});
  std::vector<double> load(4 * (bays + 1), 0.0);
  load[2 * bottom(bays) + 1] = -1000.0;
  const st::Fix wall{{bottom(0), bottom(0), top(0), top(0)}, {0, 1, 0, 1}, {}};
  const auto tip = [&](const st::StaticResult &r) { return r.displacement.select(-2, bottom(bays)).select(-1, 1).item<double>(); };

  const double reference = tip(st::solve(ladder, grid(load, 2), wall));
  st::SolveOptions short_budget;
  short_budget.cg.max_iterations = 20;
  short_budget.fail_on_nonconvergence = false;
  const auto r = st::solve(ladder, grid(load, 2), wall, short_budget);
  out.writeln("budget of 20 iterations, failure switched off: converged ", r.converged.item<bool>(), ", residual (N) ", r.residual_norm.item<double>(), ", tip deflection (m) ", tip(r), "   reference ", reference);
  short_budget.fail_on_nonconvergence = true;
  try {
    (void)st::solve(ladder, grid(load, 2), wall, short_budget);
  } catch (const std::exception &e) {
    std::cout << "with the failure switched on: " << e.what() << "\n";
  }
}
```

Output

```text
load pushing up: bar forces (N) [-3162.0553, -4940.7115, -3162.0553], stresses (MPa) [-3.1621, -4.9407, -3.1621]
budget of 20 iterations, failure switched off: converged false, residual (N) 3490.9063, tip deflection (m) -0.0008   reference -0.2142
with the failure switched on: eng::structure::solve: the stiffness solve stopped after 20 iterations without reaching its tolerance; raise cg.max_iterations or loosen cg.relative_tolerance (long or slender structures are poorly conditioned)
```

Pushing the hung joint up puts all three bars in compression, with negative forces and stresses; a negative number is a push, not an error. With a budget of twenty iterations and the failure switched off, `solve` returns a tip deflection that is wrong, with `converged` false and a large residual; with the failure switched on, the same call throws an error that names the iteration count and the setting to raise. Always check `converged` when you switch the failure off.

Reference: [`solve`](../../../../api/eng/structure/statics/solve.md), [`StaticResult`](../../../../api/eng/structure/statics/StaticResult.md), [`SolveOptions`](../../../../api/eng/structure/statics/SolveOptions.md), [`Truss`](../../../../api/eng/structure/model/TrussModel.md), [`Fix`](../../../../api/eng/structure/model/Fix.md).

Where next: [Supports, loads and sweeps](supports_loads_and_sweeps.md) covers the ways to support a structure, prescribed support motion, mechanisms, and solving many load cases and many designs in one call.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="describing_a_truss.html">
    <span class="label">Previous</span>
    <span class="title">Describing a truss</span>
  </a>
  <a class="next-link" href="supports_loads_and_sweeps.html">
    <span class="label">Next</span>
    <span class="title">Supports, loads and sweeps</span>
  </a>
</div>
<!-- apiref:nav:end -->
