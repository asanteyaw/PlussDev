<!-- generated from solve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::solve`

Solves the static equilibrium of a truss or a frame.

## Signature

```cpp
structure::StaticResult structure::solve(const structure::Truss& truss, const pluss::Series& load, const structure::Fix& fix, const structure::SolveOptions& options = {});
structure::FrameResult structure::solve(const structure::Frame& frame, const pluss::Series& load, const structure::Fix& fix, const structure::SolveOptions& options = {});
structure::FrameResult structure::solve(const structure::Frame& frame, const pluss::Series& load, const structure::MemberLoad& member_load, const structure::Fix& fix, const structure::SolveOptions& options = {});
structure::NonlinearResult structure::solve(const structure::Frame& frame, const pluss::Series& load, const structure::Fix& fix, const structure::NonlinearOptions& options);
```

## What It Does

Finds the displacements, support reactions and member results of a structure under steady loads. There are four forms. The truss form solves the linear equilibrium `K u = f` of a pin-jointed truss. The frame form does the same for a frame of beam-columns. The frame form with a `MemberLoad` also carries uniform loads along the members exactly. The last form, selected by passing `NonlinearOptions`, solves the large-displacement corotational equilibrium by load stepping and Newton iteration. The linear forms apply the stiffness bar by bar in a preconditioned conjugate-gradient iteration and never assemble a matrix; leading axes on the structure, the load and the member load are batches solved together.

## Parameters

`truss / frame`
: The compiled structure.

`load`
: Nodal loads, `[..., node, axis]` for a truss and `[..., node, 3]` for a frame, with leading axes for load cases.

`member_load`
: Uniform loads along frame members.

`fix`
: The supported degrees of freedom and any prescribed values.

`options`
: `SolveOptions` for the linear forms, `NonlinearOptions` for the large-displacement form.

## Returns

A `StaticResult` for a truss, a `FrameResult` for the linear frame forms, a `NonlinearResult` for the large-displacement form.

## Errors

Throws `std::invalid_argument` for a malformed or non-finite load, a load that does not share the structure's dtype and platform, an empty or inconsistent `Fix`, a degree of freedom named twice or out of range, or a free degree of freedom with no stiffness. Throws `std::runtime_error` when the iteration cannot finish: the message says whether the structure is a mechanism or the iteration budget ran out, and a nonlinear continuation that stops short reports its load factor. `fail_on_nonconvergence = false` returns the diagnostics instead.

## Interpretation and Use

Choose the linear forms when displacements are small; use the nonlinear form when the load turns the structure through more than a few degrees. The nodal load places forces and moments at nodes; use a `MemberLoad` for uniform loads along members, which keeps `end_force` true. Check equilibrium of loads and reactions on any result.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Solve the hung three-bar truss and read every field, with a batch of designs and load cases.

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

// the hung three-bar truss: supports at the top, the loaded joint at (0, 0)
static structure::Truss hung() {
  return structure::truss({grid({-3, 4, 0, 4, 3, 4, 0, 0}, 2), {0, 1, 2}, {3, 3, 3}}, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
}
static const structure::Fix kSupports{{0, 0, 1, 1, 2, 2}, {0, 1, 0, 1, 0, 1}, {}};

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto truss = hung();
  const auto r = structure::solve(truss, grid({0, 0, 0, 0, 0, 0, 0, -10000}, 2), kSupports);
  out.writeln("displacement of the hung joint (mm): ", 1000.0 * flat(r.displacement)[6], " ", 1000.0 * flat(r.displacement)[7]);
  out.writeln("force (N):   ", flat(r.force));
  out.writeln("stress (MPa): ", flat(r.stress.mul(1e-6)));
  out.writeln("strain (1e-6): ", flat(r.strain.mul(1e6)));
  out.writeln("strain energy (J): ", r.energy.item<double>());
  out.writeln("reaction at the middle support (N): ", flat(r.reaction)[2], " ", flat(r.reaction)[3]);
  out.writeln("residual ", r.residual_norm.item<double>(), ", converged ", r.converged.item<bool>(), ", breakdown ", r.breakdown.item<bool>(), ", iterations ", r.iterations);

  // two load cases and four middle-bar areas in one call
  const auto areas = pluss::series::make(std::vector<double>{1e-3, 1e-3, 1e-3, 1e-3, 2e-3, 1e-3, 1e-3, 4e-3, 1e-3, 1e-3, 8e-3, 1e-3}).reshape({4, 3});
  const auto designs = structure::truss({grid({-3, 4, 0, 4, 3, 4, 0, 0}, 2), {0, 1, 2}, {3, 3, 3}}, {areas, vec({200e9, 200e9, 200e9})});
  const auto loads = pluss::series::make(std::vector<double>{0, 0, 0, 0, 0, 0, 0, -10000, 0, 0, 0, 0, 0, 0, 5000, 0}).reshape({2, 1, 4, 2});
  const auto batch = structure::solve(designs, loads, kSupports);
  out.writeln("batched: force shape ", batch.force.sizes(), ", displacement shape ", batch.displacement.sizes());
}
```

Output

```text
displacement of the hung joint (mm): 0.0000 -0.0988
force (N):   [3162.0553, 4940.7115, 3162.0553]
stress (MPa): [3.1621, 4.9407, 3.1621]
strain (1e-6): [15.8103, 24.7036, 15.8103]
strain energy (J): 0.4941
reaction at the middle support (N): 0.0000 4940.7115
residual 0.0000, converged true, breakdown false, iterations 8
batched: force shape [2, 4, 3], displacement shape [2, 4, 4, 2]
```
### Example 2

Solve a cantilever frame and read the end actions.

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

Uniform member loads on a frame.

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
### Example 4

Large-displacement solution of a cantilever against the elastica.

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
### Example 5

Solver options and the failure switch.

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
### Example 6

Supports, rollers and prescribed displacements.

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
  // the triangle with a tie bar from the left foot to the right foot
  const auto truss = structure::truss({grid({0, 0, 6, 0, 3, 4}, 2), {0, 1, 0}, {2, 2, 1}}, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
  const auto load = grid({0, 0, 0, 0, 0, -10000}, 2);

  // a pin at node 0 (x and y) and a roller at node 1 (y only)
  const structure::Fix pin_and_roller{{0, 0, 1}, {0, 1, 1}, {}};
  const auto a = structure::solve(truss, load, pin_and_roller);
  out.writeln("pin and roller: right foot slides (mm) ", 1000.0 * flat(a.displacement)[2], ", tie force (N) ", flat(a.force)[2]);

  // two pins: the right foot cannot slide, and the tie carries nothing
  const structure::Fix two_pins{{0, 0, 1, 1}, {0, 1, 0, 1}, {}};
  const auto b = structure::solve(truss, load, two_pins);
  out.writeln("two pins:       right foot slides (mm) ", 1000.0 * flat(b.displacement)[2] + 0.0, ", tie force (N) ", flat(b.force)[2]);

  // a prescribed value: the right foot is also pushed 1 mm to the right (the third pair is node 1, x, value 0.001)
  const structure::Fix pushed{{0, 0, 1, 1}, {0, 1, 0, 1}, vec({0, 0, 0.001, 0})};
  const auto c = structure::solve(truss, load, pushed);
  out.writeln("prescribed 1 mm: right foot moves (mm) ", 1000.0 * flat(c.displacement)[2], ", tie force (N) ", flat(c.force)[2]);

  // the same degree of freedom named twice is rejected
  try {
    (void)structure::solve(truss, load, structure::Fix{{0, 0, 0}, {0, 1, 1}, {}});
  } catch (const std::exception &e) {
    std::cout << "a degree of freedom named twice: " << e.what() << "\n";
  }
}
```

Output

```text
pin and roller: right foot slides (mm) 0.1125, tie force (N) 3750.0000
two pins:       right foot slides (mm) 0.0000, tie force (N) 0.0000
prescribed 1 mm: right foot moves (mm) 1.0000, tie force (N) 33333.3333
a degree of freedom named twice: eng::structure::solve: constrained degrees of freedom must be unique
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="NonlinearResult.html">
    <span class="label">Previous</span>
    <span class="title">NonlinearResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
