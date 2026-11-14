<!-- generated from structure.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Structures

Plane and space trusses, plane frames, and their static, modal, buckling, time and frequency analysis. A structure is described once, as a `Mesh` with `Bars` or `Sections`, compiled into an immutable `Truss` or `Frame`, and then analysed under any number of loads and supports.

## Choosing A Surface

| Task | Branch |
|---|---|
| Describe the structure and its supports | [Structural models](structure/model.md) |
| Displacements, reactions and member forces under steady loads | [Static analysis](structure/statics.md) |
| Natural frequencies, mode shapes and buckling loads | [Mass, modes and buckling](structure/spectral.md) |
| Response in time or at each frequency | [Time and frequency response](structure/dynamics.md) |

## Practical Examples

The examples build a truss and a frame, solve the hung three-bar truss and read every field, and find the modes of a cantilever.

### Example 1

Build a plane and a space truss and a batch of three apex heights.

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
  out.print_defaults(PrintOptions{}.with_sep(""));
  // a plane truss: two bars from the feet (0, 0) and (6, 0) to the apex (3, 4)
  const structure::Mesh plane{grid({0, 0, 6, 0, 3, 4}, 2), {0, 1}, {2, 2}};
  const auto triangle = structure::truss(plane, {vec({1e-3, 1e-3}), vec({200e9, 200e9})});
  out.writeln("plane truss: nodes ", triangle.nodes(), ", bars ", triangle.bars(), ", axes ", triangle.dim());

  // a space truss: three legs from a ring on the ground to an apex 2 m above its centre
  const structure::Mesh space{grid({1.5, 0, 0, -0.75, 1.299038105676658, 0, -0.75, -1.299038105676658, 0, 0, 0, 2}, 3), {0, 1, 2}, {3, 3, 3}};
  const auto tripod = structure::truss(space, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
  out.writeln("space truss: nodes ", tripod.nodes(), ", bars ", tripod.bars(), ", axes ", tripod.dim());

  // a batch of three apex heights: a leading axis on the positions describes three trusses at once
  const auto positions = pluss::series::make(std::vector<double>{0, 0, 6, 0, 3, 2, 0, 0, 6, 0, 3, 4, 0, 0, 6, 0, 3, 8}).reshape({3, 3, 2});
  const auto batch = structure::truss({positions, {0, 1}, {2, 2}}, {vec({1e-3, 1e-3}), vec({200e9, 200e9})});
  const auto result = structure::solve(batch, grid({0, 0, 0, 0, 0, -10000}, 2), {{0, 0, 1, 1}, {0, 1, 0, 1}, {}});
  out.writeln("batch of three heights: bar forces have shape ", result.force.sizes());
}
```

Output

```text
plane truss: nodes 3, bars 2, axes 2
space truss: nodes 4, bars 3, axes 3
batch of three heights: bar forces have shape [3, 2]
```
### Example 2

Solve the hung three-bar truss and read every result field.

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
### Example 3

Find the lowest modes of a cantilever against the closed formula.

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

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

structure/model
structure/statics
structure/spectral
structure/dynamics
```
<!-- apiref:toctree:end -->
