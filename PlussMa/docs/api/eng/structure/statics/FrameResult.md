<!-- generated from FrameResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::FrameResult`

Displacements, reactions and member end actions of a frame solve.

## Signature

```cpp
structure::FrameResult
```

## What It Does

The result of `solve` on a `Frame`, linear, with or without a `MemberLoad`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `displacement` | Node displacements `(ux, uy, rz)`, `[..., node, 3]`. |  |
| `reaction` | Support forces and moments, `[..., node, 3]`, zero at free degrees of freedom. |  |
| `end_force` | Local end actions `(Ni, Vi, Mi, Nj, Vj, Mj)` of each member, `[..., member, 6]`: the axial force, shear and moment that the nodes apply to the member at its first and second end, in the member's local axes, counterclockwise moments positive. With a `MemberLoad` they include the member's own load. |  |
| `energy` | Total strain energy, `[...]`. |  |
| `residual_norm` | Out-of-balance force of the iteration, `[...]`. |  |
| `converged` | One where the iteration reached its tolerance. |  |
| `breakdown` | One where a mechanism was met. |  |
| `iterations` | The iteration count. |  |

## Returns

Returned by `solve(const Frame&, ...)` and by the overload with a `MemberLoad`.

## Errors

Reading a field does not solve again.

## Interpretation and Use

The sagging bending moment of a member is `-Mi` at its first end and `+Mj` at its second. For a tension, `Ni` is negative and `Nj` positive.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field of a cantilever solve.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::ma::eng::structure;
using namespace pluss::tils;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a straight steel beam of length 2 m along x, with density, divided into equal members
static structure::Frame beam(int members, bool with_density = true) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(2.0 * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  structure::Sections sections{vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))};
  if (with_density) sections.density = vec(std::vector<double>(members, 7850.0));
  return structure::frame({grid(position, 2), from, to}, sections);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const int members = 4;
  const auto frame = beam(members, false);
  // 100 N down at the free end of a cantilever fixed at node 0: E I = 200e9 * 8.333e-8
  std::vector<double> load(3 * (members + 1), 0.0);
  load[3 * members + 1] = -100.0;
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const auto r = structure::solve(frame, grid(load, 3), wall);

  const auto u = flat(r.displacement);
  out.writeln("tip deflection (mm) ", 1000.0 * u[3 * members + 1], "   formula -P L^3 / (3 E I) = ", -1000.0 * 100.0 * 8.0 / (3 * 200e9 * 8.333e-8));
  out.writeln("tip rotation (rad)  ", u[3 * members + 2], "   formula -P L^2 / (2 E I) = ", -100.0 * 4.0 / (2 * 200e9 * 8.333e-8));
  out.writeln("wall reaction: force ", flat(r.reaction)[1], ", moment ", flat(r.reaction)[2]);
  const auto actions = flat(r.end_force);
  out.writeln("end actions of the member at the wall (Ni Vi Mi Nj Vj Mj): ", std::vector<double>(actions.begin(), actions.begin() + 6));
  out.writeln("strain energy (J) ", r.energy.item<double>(), ", converged ", r.converged.item<bool>(), ", iterations ", r.iterations);
}
```

Output

```text
tip deflection (mm) -16.00064   formula -P L^3 / (3 E I) = -16.00064
tip rotation (rad)  -0.01200   formula -P L^2 / (2 E I) = -0.01200
wall reaction: force 100.00000, moment 200.00000
end actions of the member at the wall (Ni Vi Mi Nj Vj Mj): [0.00000, 100.00000, 200.00000, -0.00000, -100.00000, -150.00000]
strain energy (J) 0.80003, converged true, iterations 8
```
### Example 2

End actions with a uniform member load.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::ma::eng::structure;
using namespace pluss::tils;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a straight steel beam of length 2 m along x, with density, divided into equal members
static structure::Frame beam(int members, bool with_density = true) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(2.0 * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  structure::Sections sections{vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))};
  if (with_density) sections.density = vec(std::vector<double>(members, 7850.0));
  return structure::frame({grid(position, 2), from, to}, sections);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a 4 m beam in two members, pinned at the left end and on a roller at the right end
  std::vector<double> position = {0, 0, 2, 0, 4, 0};
  const structure::Frame frame = structure::frame({grid(position, 2), {0, 1}, {1, 2}}, {vec({1e-3, 1e-3}), vec({1e-7, 1e-7}), vec({200e9, 200e9})});
  const structure::Fix supports{{0, 0, 2}, {0, 1, 1}, {}};
  const auto no_nodal_load = pluss::series::zeros({3, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));

  // transverse: 1000 N/m downward on both members (local y points up for a member running left to right)
  const auto sagging = structure::solve(frame, no_nodal_load, structure::MemberLoad{vec({-1000.0, -1000.0}), {}}, supports);
  out.writeln("uniform load: mid-span deflection (mm) ", 1000.0 * flat(sagging.displacement)[4], "   formula ", -1000.0 * 5 * 1000.0 * 256.0 / (384 * 200e9 * 1e-7));
  out.writeln("  reactions (N) ", flat(sagging.reaction)[1], " ", flat(sagging.reaction)[7], ";  moment at mid-span from the end action (N m) ", flat(sagging.end_force)[5], "   formula ", 1000.0 * 16.0 / 8.0);

  // load on one member only: the other carries none
  const auto half = structure::solve(frame, no_nodal_load, structure::MemberLoad{vec({-1000.0, 0.0}), {}}, supports);
  out.writeln("load on the first member only: reactions (N) ", flat(half.reaction)[1], " ", flat(half.reaction)[7], "   formula ", 1000.0 * 2.0 * 3.0 / 4.0, " ", 1000.0 * 2.0 * 1.0 / 4.0);

  // axial: a vertical bar hanging from the ceiling under its own weight, 2000 N/m along the member axis (which points down)
  const auto bar = structure::frame({grid({0, 0, 0, -3}, 2), {0}, {1}}, {vec({1e-3}), vec({8e-8}), vec({200e9})});
  const auto hanging = structure::solve(bar, pluss::series::zeros({2, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)), structure::MemberLoad{{}, vec({2000.0})}, {{0, 0, 0}, {0, 1, 2}, {}});
  out.writeln("hanging bar: tip drop (mm) ", -1000.0 * flat(hanging.displacement)[4], "   formula ", 1000.0 * 2000.0 * 9.0 / (2 * 200e9 * 1e-3), ";  ceiling reaction (N) ", flat(hanging.reaction)[1]);

  // both members together, and a batch of three intensities
  const auto intensities = pluss::series::make(std::vector<double>{-500, -500, -1000, -1000, -2000, -2000}).reshape({3, 2});
  const auto batch = structure::solve(frame, no_nodal_load, structure::MemberLoad{intensities, {}}, supports);
  out.writeln("batch of three intensities: mid-span deflection (mm) ", flat(batch.displacement.select(-2, 1).select(-1, 1).mul(1000.0)));
}
```

Output

```text
uniform load: mid-span deflection (mm) -166.6667   formula -166.6667
  reactions (N) 2000.0000 2000.0000;  moment at mid-span from the end action (N m) 2000.0000   formula 2000.0000
load on the first member only: reactions (N) 1500.0000 500.0000   formula 1500.0000 500.0000
hanging bar: tip drop (mm) 0.0450   formula 0.0450;  ceiling reaction (N) 6000.0000
batch of three intensities: mid-span deflection (mm) [-83.3333, -166.6667, -333.3333]
```
### Example 3

The diagnostics under a short budget.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::ma::eng::structure;
using namespace pluss::tils;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a straight steel beam of length 2 m along x, with density, divided into equal members
static structure::Frame beam(int members, bool with_density = true) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(2.0 * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  structure::Sections sections{vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))};
  if (with_density) sections.density = vec(std::vector<double>(members, 7850.0));
  return structure::frame({grid(position, 2), from, to}, sections);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  // a cantilever ladder truss of 40 bays: long, so the stiffness solve needs a couple of hundred iterations
  const int bays = 40;
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int level = 0; level < 2; ++level)
    for (int i = 0; i <= bays; ++i) {
      position.push_back(i);
      position.push_back(level);
    }
  const auto top = [&](int i) { return bays + 1 + i; };
  for (int i = 0; i < bays; ++i) {
    from.push_back(i); to.push_back(i + 1);
    from.push_back(top(i)); to.push_back(top(i + 1));
    from.push_back(i); to.push_back(top(i + 1));
  }
  for (int i = 0; i <= bays; ++i) { from.push_back(i); to.push_back(top(i)); }
  const auto bars = static_cast<std::int64_t>(from.size());
  const auto ladder = structure::truss({grid(position, 2), from, to}, {vec(std::vector<double>(bars, 1e-3)), vec(std::vector<double>(bars, 200e9))});
  std::vector<double> load(4 * (bays + 1), 0.0);
  load[2 * bays + 1] = -1000.0;
  const structure::Fix wall{{0, 0, top(0), top(0)}, {0, 1, 0, 1}, {}};
  const auto tip = [&](const structure::StaticResult &r) { return r.displacement.select(-2, bays).select(-1, 1).item<double>(); };

  structure::SolveOptions tightest;
  tightest.cg.relative_tolerance = 1e-14;
  tightest.cg.absolute_tolerance = 1e-30;
  const double reference = tip(structure::solve(ladder, grid(load, 2), wall, tightest));
  out.writeln("tip deflection (m), tightest solve: ", reference);

  for (double tolerance : {1e-10, 1e-3}) {
    structure::SolveOptions options;
    options.cg.relative_tolerance = tolerance;
    options.cg.absolute_tolerance = 1e-30;
    const auto r = structure::solve(ladder, grid(load, 2), wall, options);
    out.writeln("cg.relative_tolerance ", tolerance, ": iterations ", r.iterations, ", residual ", r.residual_norm.item<double>(), ", tip error ", std::abs(tip(r) - reference));
  }

  // cg.max_iterations and fail_on_nonconvergence: both states
  structure::SolveOptions short_budget;
  short_budget.cg.max_iterations = 20;
  short_budget.fail_on_nonconvergence = false;
  const auto cut = structure::solve(ladder, grid(load, 2), wall, short_budget);
  out.writeln("budget of 20 iterations, failure off: converged ", cut.converged.item<bool>(), ", residual ", cut.residual_norm.item<double>());
  short_budget.fail_on_nonconvergence = true;
  try {
    (void)structure::solve(ladder, grid(load, 2), wall, short_budget);
  } catch (const std::exception &e) {
    std::cout << "budget of 20 iterations, failure on: " << e.what() << "\n";
  }
}
```

Output

```text
tip deflection (m), tightest solve: -2.14e-01
cg.relative_tolerance 1.00e-10: iterations 240, residual 5.51e-08, tip error 2.16e-15
cg.relative_tolerance 1.00e-03: iterations 176, residual 9.74e-01, tip error 1.00e-10
budget of 20 iterations, failure off: converged false, residual 3.49e+03
budget of 20 iterations, failure on: eng::structure::solve: the stiffness solve stopped after 20 iterations without reaching its tolerance; raise cg.max_iterations or loosen cg.relative_tolerance (long or slender structures are poorly conditioned)
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="StaticResult.html">
    <span class="label">Previous</span>
    <span class="title">StaticResult</span>
  </a>
  <a class="next-link" href="MemberLoad.html">
    <span class="label">Next</span>
    <span class="title">MemberLoad</span>
  </a>
</div>
<!-- apiref:nav:end -->
