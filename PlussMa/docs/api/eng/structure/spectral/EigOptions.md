<!-- generated from EigOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::EigOptions`

Controls of the modal and buckling eigenvalue searches.

## Signature

```cpp
structure::EigOptions
```

## What It Does

Controls for `modes` and `buckling`, which find the lowest few eigenpairs of a frame by an iteration that applies the stiffness inverse through inner solves.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `count` | How many modes to find, at most the number of free degrees of freedom. | `1` |
| `max_iterations` | Iterations allowed to the subspace search. A pair that has not reached its tolerance within it is flagged in `converged`. | `64` |
| `relative_tolerance` | Relative convergence tolerance of each pair. | `1e-8` |
| `absolute_tolerance` | Absolute convergence tolerance. | `1e-10` |
| `solve` | Conjugate-gradient controls of the inner stiffness solves (budget 8192, relative tolerance `1e-10`). |  |

## Returns

A value; validated by `modes` and `buckling`.

## Errors

A non-positive `count` or `max_iterations`, negative or non-finite tolerances, or a `count` above the number of free degrees of freedom are rejected.

## Interpretation and Use

Ask for no more modes than the mesh can represent. A pair that has not reached its tolerance is flagged in `converged`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Count, tolerance and iteration budget on a cantilever.

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
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};

  // mass(): a unit acceleration along y at every node takes a total force equal to the mass, 7850 * 1e-3 * 2 = 15.7 kg
  std::vector<double> acceleration(3 * (members + 1), 0.0);
  for (int node = 0; node <= members; ++node) acceleration[3 * node + 1] = 1.0;
  const auto force = structure::mass(frame, grid(acceleration, 3));
  double total = 0.0;
  for (int node = 0; node <= members; ++node) total += force.select(-2, node).select(-1, 1).item<double>();
  out.writeln("total mass from mass() (kg): ", total);

  // the lowest three modes: omega, hertz, residuals
  structure::EigOptions options;
  options.count = 3;
  const auto modes = structure::modes(frame, wall, options);
  out.writeln("omega (rad/s): ", flat(modes.omega), "   exact ", 1.875104 * 1.875104 * 11.519, " ", 4.694091 * 4.694091 * 11.519, " ", 7.854757 * 7.854757 * 11.519);
  out.writeln("hertz: ", flat(modes.hertz), ", converged ", flat(modes.converged), ", iterations ", modes.iterations);
  out.writeln("shape array ", modes.shape.sizes(), ", residuals ", flat(modes.residual));

  // mass normalisation: phi_i' M phi_j is the identity
  double worst = 0.0;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      const double product = structure::mass(frame, modes.shape.select(0, i)).mul(modes.shape.select(0, j)).sum().item<double>();
      worst = std::max(worst, std::abs(product - (i == j ? 1.0 : 0.0)));
    }
  out.writeln("largest deviation of phi_i' M phi_j from the identity: ", worst);

  // a looser tolerance and a small iteration budget
  structure::EigOptions loose;
  loose.count = 3;
  loose.relative_tolerance = 1e-3;
  const auto rough = structure::modes(frame, wall, loose);
  out.writeln("relative_tolerance 1e-3: omega ", flat(rough.omega), ", iterations ", rough.iterations);
  structure::EigOptions tiny;
  tiny.count = 3;
  tiny.max_iterations = 2;
  const auto short_run = structure::modes(frame, wall, tiny);
  out.writeln("max_iterations 2: converged ", flat(short_run.converged));
}
```

Output

```text
total mass from mass() (kg): 15.700
omega (rad/s): [40.502, 253.839, 711.132]   exact 40.501 253.815 710.690
hertz: [6.446, 40.400, 113.180], converged [1.000, 1.000, 1.000], iterations 15
shape array [3, 9, 3], residuals [0.000, 0.000, 0.003]
largest deviation of phi_i' M phi_j from the identity: 0.000
relative_tolerance 1e-3: omega [40.502, 253.839, 711.133], iterations 6
max_iterations 2: converged [0.000, 0.000, 0.000]
```
### Example 2

The absolute tolerance on its own.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <algorithm>
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
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;

  // EigOptions: the absolute tolerance on its own
  structure::EigOptions defaults;
  defaults.count = 3;
  structure::EigOptions absolute = defaults;
  absolute.relative_tolerance = 0.0;
  absolute.absolute_tolerance = 1e-3;
  for (const auto &run : {std::make_pair("defaults                ", defaults), std::make_pair("absolute tolerance 1e-3", absolute)}) {
    const auto r = structure::modes(frame, wall, run.second);
    out.writeln(run.first, ": iterations ", r.iterations, ", residuals ", flat(r.residual), ", converged ", flat(r.converged));
  }

  // MotionOptions: a starved inner solve, with the failure switch in both states
  const int steps = 20;
  std::vector<double> times, history;
  for (int i = 0; i <= steps; ++i) {
    times.push_back(0.001 * i);
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = -100.0;
    history.insert(history.end(), load.begin(), load.end());
  }
  const auto load = pluss::series::make(history).reshape({steps + 1, members + 1, 3});
  structure::MotionOptions starved;
  starved.solve.max_iterations = 2;
  starved.fail_on_nonconvergence = false;
  const auto lenient = structure::motion(frame, load, vec(times), wall, {}, starved);
  const auto flags = flat(lenient.converged);
  const auto converged_steps = std::count_if(flags.begin(), flags.end(), [](double c) { return c > 0.5; });
  out.writeln("inner solve limited to 2 iterations, failure off: converged steps ", converged_steps, " of ", steps + 1);
  starved.fail_on_nonconvergence = true;
  try {
    (void)structure::motion(frame, load, vec(times), wall, {}, starved);
  } catch (const std::exception &e) {
    std::cout << "inner solve limited to 2 iterations, failure on: " << e.what() << "\n";
  }

  // HarmonicOptions.solve: the GMRES controls
  std::vector<double> loads;
  for (int f = 0; f < 3; ++f) {
    std::vector<double> l(3 * (members + 1), 0.0);
    l[tip] = 10.0;
    loads.insert(loads.end(), l.begin(), l.end());
  }
  const auto harmonic_load = pluss::series::make(loads).reshape({3, members + 1, 3});
  structure::HarmonicOptions defaults_h;
  defaults_h.stiffness_damping = 1e-4;
  structure::HarmonicOptions small_restart = defaults_h;
  small_restart.solve.restart = 4;
  small_restart.fail_on_nonconvergence = false;
  structure::HarmonicOptions loose = defaults_h;
  loose.solve.relative_tolerance = 1e-2;
  structure::HarmonicOptions capped = defaults_h;
  capped.solve.max_iterations = 3;
  capped.fail_on_nonconvergence = false;
  for (const auto &run : {std::make_pair("defaults         ", defaults_h), std::make_pair("restart 4        ", small_restart), std::make_pair("tolerance 1e-2   ", loose), std::make_pair("max_iterations 3 ", capped)}) {
    const auto r = structure::harmonic(frame, harmonic_load, vec({10.0, 40.5, 100.0}), wall, run.second);
    out.writeln(run.first, ": iterations ", r.iterations, ", operator evaluations ", r.operator_evaluations, ", restarts ", r.restarts, ", converged ", r.converged.all().item<bool>(), ", resonance amplitude (mm) ", 1000.0 * flat(r.displacement.select(-2, members).select(-1, 1).abs())[1]);
  }
}
```

Output

```text
defaults                : iterations 15, residuals [1.77e-07, 6.63e-06, 2.51e-03], converged [1.00e+00, 1.00e+00, 1.00e+00]
absolute tolerance 1e-3: iterations 11, residuals [8.31e-08, 5.83e-06, 5.50e-01], converged [1.00e+00, 1.00e+00, 1.00e+00]
inner solve limited to 2 iterations, failure off: converged steps 0 of 21
inner solve limited to 2 iterations, failure on: eng::structure::motion: initial acceleration solve did not converge
defaults         : iterations 24, operator evaluations 26, restarts 1, converged true, resonance amplitude (mm) 3.83e+02
restart 4        : iterations 128, operator evaluations 161, restarts 32, converged false, resonance amplitude (mm) 4.67e-02
tolerance 1e-2   : iterations 24, operator evaluations 26, restarts 1, converged true, resonance amplitude (mm) 3.83e+02
max_iterations 3 : iterations 3, operator evaluations 5, restarts 1, converged false, resonance amplitude (mm) 1.12e-03
```
### Example 3

Four end conditions of a column with default options.

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
  out.print_defaults(PrintOptions{}.fixed(1).with_sep(""));
  const double euler = M_PI * M_PI * 200e9 * 8.333e-8 / 4.0;
  out.writeln("pi^2 E I / L^2 = ", euler, " N");
  const int members = 16;
  const auto column = beam(members, false);
  std::vector<double> load(3 * (members + 1), 0.0);
  load[3 * members] = -1.0;   // a 1 N reference load: the factor is the critical load in newtons
  struct Case { const char *name; structure::Fix fix; double units; };
  const Case cases[] = {
      {"cantilever            ", {{0, 0, 0}, {0, 1, 2}, {}}, 0.25},
      {"pinned-pinned         ", {{0, 0, members}, {0, 1, 1}, {}}, 1.0},
      {"fixed-pinned          ", {{0, 0, 0, members}, {0, 1, 2, 1}, {}}, 2.0457},
      {"fixed-fixed (sliding) ", {{0, 0, 0, members, members}, {0, 1, 2, 1, 2}, {}}, 4.0},
  };
  for (const Case &c : cases) {
    const auto r = structure::buckling(column, grid(load, 3), c.fix);
    out.writeln(c.name, "critical load (N) ", flat(r.factor)[0], "   Euler ", c.units * euler);
  }
}
```

Output

```text
pi^2 E I / L^2 = 41121.7 N
cantilever            critical load (N) 10280.4   Euler 10280.4
pinned-pinned         critical load (N) 41121.8   Euler 41121.7
fixed-pinned          critical load (N) 84125.4   Euler 84122.7
fixed-fixed (sliding) critical load (N) 164492.2   Euler 164486.8
```
### Example 4

Rejected counts.

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

static void report(const char *what, const std::function<void()> &call) {
  try {
    call();
    std::cout << what << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << what << ": " << e.what() << "\n";
  }
}

int main() {
  const int members = 4;
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  std::vector<double> push(3 * (members + 1), 0.0);
  push[3 * members] = -1000.0;
  report("modes without density      ", [&] { (void)structure::modes(beam(members, false), wall); });
  report("mass without density       ", [&] { (void)structure::mass(beam(members, false), grid(std::vector<double>(3 * (members + 1), 0.0), 3)); });
  report("more modes than free dofs   ", [&] { structure::EigOptions o; o.count = 50; (void)structure::modes(beam(members), wall, o); });
  report("zero modes requested        ", [&] { structure::EigOptions o; o.count = 0; (void)structure::modes(beam(members), wall, o); });
  report("buckling, tensile reference ", [&] { for (auto &v : push) v = -v; (void)structure::buckling(beam(members, false), grid(push, 3), {{0, 0, members}, {0, 1, 1}, {}}); });
}
```

Output

```text
modes without density      : eng::structure::modes: frame sections do not define density
mass without density       : eng::structure::mass: frame sections do not define density
more modes than free dofs   : eng::structure::modes: count exceeds the free degree count
zero modes requested        : eng::structure::modes: count and max_iterations must both be positive
buckling, tensile reference : eng::structure::buckling: the reference load puts no member in compression, so there is no buckling factor; apply a compressive axial reference load (a zero, tensile or purely transverse load never buckles a frame in a linearised analysis)
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="ModeResult.html">
    <span class="label">Next</span>
    <span class="title">ModeResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
