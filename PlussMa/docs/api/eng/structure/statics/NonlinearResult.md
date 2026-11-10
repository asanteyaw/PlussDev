<!-- generated from NonlinearResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::NonlinearResult`

Displacements, reactions and end actions of a large-displacement solve.

## Signature

```cpp
structure::NonlinearResult
```

## What It Does

The result of `solve(frame, load, fix, NonlinearOptions)`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `displacement` | Node displacements `(ux, uy, rz)` from the original positions, `[..., node, 3]`. |  |
| `reaction` | Support force and moment in the fixed global axes, `[..., node, 3]`. |  |
| `end_force` | End actions `(Ni, Vi, Mi, Nj, Vj, Mj)` of each member in its current, turned local axes, `[..., member, 6]`. |  |
| `energy` | Strain energy, `[...]`. |  |
| `residual_norm` | Out-of-balance force at the end, `[...]`. |  |
| `converged` | One where the final residual is within tolerance. |  |
| `load_factor` | The fraction of the load reached, `1` for a complete solve. |  |
| `iterations` | Newton iterations in all. |  |
| `steps` | Load steps taken. |  |
| `cutbacks` | Step halvings forced. |  |

## Returns

Returned by the nonlinear overload of `solve`.

## Errors

Reading a field does not solve again.

## Interpretation and Use

A `load_factor` below one means the continuation stopped short, and `converged` is then false. End actions refer to each member's current axes, reactions to the global axes.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field on a cantilever bent through a large angle.

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
  const int members = 16;
  const auto frame = beam(members, false);
  const double load = 4999.8;   // alpha = P L^2 / (E I) = 1.2 for the 2 m beam: the elastica gives a tip deflection of 0.3490 L
  std::vector<double> nodal(3 * (members + 1), 0.0);
  nodal[3 * members + 1] = -load;
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};

  structure::NonlinearOptions options;
  options.load_steps = 20;
  const auto r = structure::solve(frame, grid(nodal, 3), wall, options);
  out.writeln("tip down / L ", -flat(r.displacement)[3 * members + 1] / 2.0, " (elastica 0.3490), inward / L ", -flat(r.displacement)[3 * members] / 2.0, " (elastica 0.0764)");
  out.writeln("converged ", r.converged.item<bool>(), ", load factor ", r.load_factor, ", steps ", r.steps, ", cutbacks ", r.cutbacks, ", Newton iterations ", r.iterations);
  out.writeln("residual ", r.residual_norm.item<double>(), ", strain energy (J) ", r.energy.item<double>());
  out.writeln("end actions of the member at the wall, in its own turned axes (Ni Vi Mi): ", flat(r.end_force)[0], " ", flat(r.end_force)[1], " ", flat(r.end_force)[2]);

  // a tighter tolerance and a different number of steps
  structure::NonlinearOptions tight = options;
  tight.relative_tolerance = 1e-12;
  tight.load_steps = 5;
  const auto s = structure::solve(frame, grid(nodal, 3), wall, tight);
  out.writeln("tolerance 1e-12, 5 steps: tip down / L ", -flat(s.displacement)[3 * members + 1] / 2.0, ", iterations ", s.iterations, ", cutbacks ", s.cutbacks);

  // iterations capped at 1 per step with the failure switched off, then on
  structure::NonlinearOptions capped = options;
  capped.max_iterations = 1;
  capped.max_cutbacks = 2;
  capped.fail_on_nonconvergence = false;
  const auto t = structure::solve(frame, grid(nodal, 3), wall, capped);
  out.writeln("one iteration per step, failure off: load factor reached ", t.load_factor, ", converged ", t.converged.item<bool>());
  capped.fail_on_nonconvergence = true;
  try {
    (void)structure::solve(frame, grid(nodal, 3), wall, capped);
  } catch (const std::exception &e) {
    std::cout << "one iteration per step, failure on: " << e.what() << "\n";
  }
}
```

Output

```text
tip down / L 0.3491 (elastica 0.3490), inward / L 0.0763 (elastica 0.0764)
converged true, load factor 1.0000, steps 20, cutbacks 0, Newton iterations 150
residual 0.0000, strain energy (J) 1628.4207
end actions of the member at the wall, in its own turned axes (Ni Vi Mi): -169.2417 4996.9348 9236.1541
tolerance 1e-12, 5 steps: tip down / L 0.3491, iterations 84, cutbacks 0
one iteration per step, failure off: load factor reached 0.0000, converged false
one iteration per step, failure on: eng::structure::solve: nonlinear equilibrium did not converge before the load-step cutback limit
```
### Example 2

The linear result of the same frame.

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
### Example 3

Member loads on the linear solve.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="NonlinearOptions.html">
    <span class="label">Previous</span>
    <span class="title">NonlinearOptions</span>
  </a>
  <a class="next-link" href="solve.html">
    <span class="label">Next</span>
    <span class="title">solve</span>
  </a>
</div>
<!-- apiref:nav:end -->
