<!-- generated from StaticResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::StaticResult`

Displacements, reactions and bar results of a truss solve.

## Signature

```cpp
structure::StaticResult
```

## What It Does

The result of `solve` on a `Truss`. Arrays keep the leading batch axes of the structure and the load.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `displacement` | Node displacements, `[..., node, axis]`. |  |
| `reaction` | Support forces, `[..., node, axis]`, zero at free degrees of freedom. |  |
| `strain` | Engineering strain of each bar, `[..., bar]`. |  |
| `stress` | Axial stress of each bar, `force / area`, `[..., bar]`. |  |
| `force` | Axial force of each bar, `[..., bar]`, positive in tension. |  |
| `energy` | Total strain energy, `[...]`. |  |
| `residual_norm` | Size of the force left unbalanced by the iteration, `[...]`. |  |
| `converged` | One where the iteration reached its tolerance, `[...]`. |  |
| `breakdown` | One where the iteration met a direction of zero stiffness (a mechanism), `[...]`. |  |
| `iterations` | The iteration count, a number. |  |

## Returns

Returned by `solve(const Truss&, ...)`.

## Errors

Reading a field does not solve again.

## Interpretation and Use

Check `converged` before using the numbers when the solve was run with `fail_on_nonconvergence` false. Reactions are the forces the supports apply; equilibrium of the loads and reactions is a check that needs no known answer.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field of a truss solve on the three-bar truss.

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

The diagnostic fields under a short iteration budget.

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
### Example 3

Prescribed displacements and the reactions they produce.

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
  <a class="prev-link" href="SolveOptions.html">
    <span class="label">Previous</span>
    <span class="title">SolveOptions</span>
  </a>
  <a class="next-link" href="FrameResult.html">
    <span class="label">Next</span>
    <span class="title">FrameResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
