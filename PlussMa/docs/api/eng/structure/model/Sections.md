<!-- generated from Sections.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::Sections`

Cross-section properties and density of each frame member.

## Signature

```cpp
structure::Sections
```

## What It Does

A `Sections` holds what a frame member needs besides its geometry: area for axial stiffness, the second moment of area for bending stiffness, the modulus and, optionally, the density that gives the member mass.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `area` | Cross-sectional area `A` of each member, shape `[..., member]`, positive. Axial stiffness is `E A / L`. | required |
| `inertia` | Second moment of area `I` of each member about the axis perpendicular to the plane of the frame, shape `[..., member]`, positive. Bending stiffness scales with `E I / L^3`. | required |
| `modulus` | Young's modulus `E` of each member, shape `[..., member]`, positive. | required |
| `density` | Mass per volume of each member, shape `[..., member]`, positive. It is optional and is needed only by `mass`, `modes`, `motion` and `harmonic`. | undefined (no mass) |

## Returns

A value; validated by `frame`.

## Errors

Non-positive entries, a wrong member count, or a dtype or platform different from the mesh are rejected by `frame`. A density that is present must be positive.

## Interpretation and Use

Compute `I` for the actual cross-section and bending axis: a rectangle `b` wide and `d` deep has `I = b d^3 / 12` for bending in the plane of the depth. Leave `density` undefined for static analysis; `modes`, `mass`, `motion` and `harmonic` refuse a frame without it and say so.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Sections with and without density, and the total mass the density implies.

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
### Example 2

Rejected section data, including a zero density.

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
### Example 3

Modes of a frame whose sections carry a density.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Bars.html">
    <span class="label">Previous</span>
    <span class="title">Bars</span>
  </a>
  <a class="next-link" href="Fix.html">
    <span class="label">Next</span>
    <span class="title">Fix</span>
  </a>
</div>
<!-- apiref:nav:end -->
