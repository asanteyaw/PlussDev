<!-- generated from model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Structural models

This branch describes a structure. A `Mesh` and `Bars` become a `Truss` through `truss`; a `Mesh` and `Sections` become a `Frame` through `frame`. A `Fix` names the supports, and is passed to every analysis.

## Choosing A Surface

Truss bars carry only axial force and are joined at pins, in a plane or in space; frame members carry axial force, shear and bending and are joined rigidly, in a plane. Choose a truss when the members are slender and loaded only at the joints.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Build a plane and a space truss and a batch of heights.

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

Build a portal frame with and without density.

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
  out.print_defaults(PrintOptions{}.with_sep(""));
  // a portal frame: nodes 0 left base, 1 left top, 2 right top, 3 right base; members: left column, beam, right column
  const structure::Mesh portal{grid({0, 0, 0, 3, 4, 3, 4, 0}, 2), {0, 1, 3}, {1, 2, 2}};
  const structure::Sections sections{vec({2e-2, 2e-2, 2e-2}), vec({6.667e-5, 6.667e-5, 6.667e-5}), vec({200e9, 200e9, 200e9})};
  const auto frame = structure::frame(portal, sections);
  out.writeln("without density: nodes ", frame.nodes(), ", members ", frame.members(), ", degrees of freedom per node ", frame.dofs());

  // the density is optional; with it the frame has mass, and mass() returns the inertial force of an acceleration
  structure::Sections with_mass = sections;
  with_mass.density = vec({7850.0, 7850.0, 7850.0});
  const auto heavy = structure::frame(portal, with_mass);
  std::vector<double> gravity(12, 0.0);
  for (int node = 0; node < 4; ++node) gravity[3 * node + 1] = 1.0;
  const auto force = structure::mass(heavy, grid(gravity, 3));
  double total = 0.0;
  for (int node = 0; node < 4; ++node) total += force.select(-2, node).select(-1, 1).item<double>();
  out.writeln("total mass from mass() (kg): ", total, "   rho A (2 L_column + L_beam) = ", 7850.0 * 2e-2 * (3 + 4 + 3));
}
```

Output

```text
without density: nodes 4, members 3, degrees of freedom per node 3
total mass from mass() (kg): 1570   rho A (2 L_column + L_beam) = 1570
```
### Example 3

Pins, rollers, prescribed displacements and a repeated degree of freedom.

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
```{toctree}
:maxdepth: 2
:hidden:

model/Mesh
model/Bars
model/Sections
model/Fix
model/TrussModel
model/FrameModel
model/truss
model/frame
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

model/Mesh
model/Bars
model/Sections
model/Fix
model/TrussModel
model/FrameModel
model/truss
model/frame
```
<!-- apiref:toctree:end -->
