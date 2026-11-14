<!-- generated from NonlinearOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::NonlinearOptions`

Controls of the large-displacement static solve.

## Signature

```cpp
structure::NonlinearOptions
```

## What It Does

The corotational static solve applies the load in steps and solves each with Newton's method and a line search. `NonlinearOptions` sets the steps, the iteration limits and tolerances.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `load_steps` | The number of equal load increments to aim for. A step that fails is halved and retried; after a success the increment grows again. | `4` |
| `max_iterations` | Newton iterations allowed per load step. | `24` |
| `max_line_search` | Step reductions allowed in the line search of one iteration. | `10` |
| `max_cutbacks` | Consecutive halvings of the load step allowed before the continuation gives up. | `8` |
| `relative_tolerance` | Out-of-balance force as a fraction of the load at which a step is accepted. | `1e-8` |
| `absolute_tolerance` | Out-of-balance force below which a step is accepted. A tolerance below the rounding floor of the internal forces is raised to that floor. | `1e-10` |
| `line_search_shrink` | Factor by which a rejected Newton step is shortened, in `(0, 1)`. | `0.5` |
| `armijo` | Fraction of the predicted decrease that a Newton step must achieve, in `(0, 1)`. | `1e-4` |
| `min_load_step` | Smallest load increment, as a fraction of the load; a continuation that needs a smaller one stops. | `1e-6` |
| `solve` | Conjugate-gradient controls of the inner linear solves (`max_iterations`, tolerances, `check_interval`). Loss of positive curvature is reported as nonconvergence rather than hidden behind a regularisation. | budget 8192 |
| `fail_on_nonconvergence` | When true a continuation that does not reach the full load throws; when false it returns the last equilibrium with `load_factor` below one. | `true` |

## Returns

A value; validated by the solver.

## Errors

A non-positive `load_steps`, `max_iterations` or `max_line_search`, negative tolerances, a shrink or `armijo` outside `(0, 1)`, or a `min_load_step` outside `(0, 1]` is rejected before solving.

## Interpretation and Use

More requested steps usually cost fewer Newton iterations in total, since each starts nearer its answer. With `fail_on_nonconvergence` false, check `load_factor` and `converged`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Steps, tolerance, an iteration cap and the failure switch on a cantilever bent through a large angle.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::subsp::eng::structure;
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

Line-search, step-size and cutback controls.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::subsp::eng::structure;
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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const auto truss = hung();
  const structure::Fix supports{{0, 0, 1, 1, 2, 2}, {0, 1, 0, 1, 0, 1}, {}};
  const auto load = grid({0, 0, 0, 0, 0, 0, 0, -10000}, 2);
  const auto run = [&](const char *label, const structure::SolveOptions &o) {
    const auto r = structure::solve(truss, load, supports, o);
    out.writeln(label, ": iterations ", r.iterations, ", residual ", r.residual_norm.item<double>(), ", converged ", r.converged.item<bool>(), ", breakdown ", r.breakdown.item<bool>());
  };
  structure::SolveOptions defaults;
  run("defaults                       ", defaults);
  structure::SolveOptions absolute = defaults;
  absolute.cg.relative_tolerance = 0.0;
  absolute.cg.absolute_tolerance = 1e-3;
  run("absolute tolerance 1e-3 only   ", absolute);
  structure::SolveOptions every = defaults;
  every.cg.check_interval = 1;
  run("check_interval 1               ", every);
  structure::SolveOptions curvature = defaults;
  curvature.cg.curvature_tolerance = 1e30;
  curvature.fail_on_nonconvergence = false;
  run("curvature_tolerance 1e30       ", curvature);

  // large-displacement controls on a cantilever bent to alpha = 1.2
  const int members = 16;
  const auto frame = beam(members, false);
  std::vector<double> nodal(3 * (members + 1), 0.0);
  nodal[3 * members + 1] = -4999.8;
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const auto bend = [&](const char *label, const structure::NonlinearOptions &o) {
    const auto r = structure::solve(frame, grid(nodal, 3), wall, o);
    out.writeln(label, ": converged ", r.converged.item<bool>(), ", load factor ", r.load_factor, ", iterations ", r.iterations, ", cutbacks ", r.cutbacks);
  };
  structure::NonlinearOptions base;
  base.load_steps = 20;
  base.fail_on_nonconvergence = false;
  bend("defaults                      ", base);
  structure::NonlinearOptions one_search = base;
  one_search.max_line_search = 1;
  bend("max_line_search 1             ", one_search);
  structure::NonlinearOptions gentle = base;
  gentle.line_search_shrink = 0.9;
  bend("line_search_shrink 0.9        ", gentle);
  structure::NonlinearOptions strict = base;
  strict.armijo = 0.5;
  bend("armijo 0.5                    ", strict);
  structure::NonlinearOptions stalled = base;
  stalled.max_iterations = 1;
  stalled.max_cutbacks = 1000;
  stalled.min_load_step = 1e-6;
  bend("1 iteration, min step 1e-6    ", stalled);
  stalled.min_load_step = 0.1;
  bend("1 iteration, min step 0.1     ", stalled);
}
```

Output

```text
defaults                       : iterations 8, residual 0.00e+00, converged true, breakdown false
absolute tolerance 1e-3 only   : iterations 8, residual 0.00e+00, converged true, breakdown false
check_interval 1               : iterations 1, residual 0.00e+00, converged true, breakdown false
curvature_tolerance 1e30       : iterations 0, residual 1.00e+04, converged false, breakdown true
defaults                      : converged true, load factor 1.00e+00, iterations 150, cutbacks 0
max_line_search 1             : converged true, load factor 1.00e+00, iterations 333, cutbacks 66
line_search_shrink 0.9        : converged true, load factor 1.00e+00, iterations 136, cutbacks 0
armijo 0.5                    : converged true, load factor 1.00e+00, iterations 150, cutbacks 0
1 iteration, min step 1e-6    : converged false, load factor 0.00e+00, iterations 16, cutbacks 16
1 iteration, min step 0.1     : converged false, load factor 0.00e+00, iterations 1, cutbacks 1
```
### Example 3

The linear solve for comparison.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::subsp::eng::structure;
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
### Example 4

Validation raised by the frame.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::subsp::eng::structure;
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

#include <functional>

static void report(const char *what, const std::function<void()> &build) {
  try {
    build();
    std::cout << what << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << what << ": " << e.what() << "\n";
  }
}

int main() {
  const auto line = grid({0, 0, 1, 0}, 2);
  const auto area = vec({1e-3}), inertia = vec({8e-8}), modulus = vec({200e9});
  report("coordinates in 3D  ", [&] { (void)structure::frame({grid({0, 0, 0, 1, 0, 0}, 3), {0}, {1}}, {area, inertia, modulus}); });
  report("negative inertia   ", [&] { (void)structure::frame({line, {0}, {1}}, {area, vec({-8e-8}), modulus}); });
  report("wrong inertia count", [&] { (void)structure::frame({line, {0}, {1}}, {area, vec({8e-8, 8e-8}), modulus}); });
  report("zero density       ", [&] { (void)structure::frame({line, {0}, {1}}, {area, inertia, modulus, vec({0.0})}); });
}
```

Output

```text
coordinates in 3D  : eng::structure::frame: a frame needs at least two nodes and exactly two spatial axes
negative inertia   : eng::structure::frame: area, inertia, and modulus must be positive
wrong inertia count: eng::structure::frame: area, inertia, and modulus must end in the member count
zero density       : eng::structure::frame: density must be positive
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="MemberLoad.html">
    <span class="label">Previous</span>
    <span class="title">MemberLoad</span>
  </a>
  <a class="next-link" href="NonlinearResult.html">
    <span class="label">Next</span>
    <span class="title">NonlinearResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
