<!-- generated from spectral.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Mass, modes and buckling

This branch treats the eigenproblems of a frame. `mass` applies the consistent mass, `modes` finds natural frequencies and shapes, and `buckling` finds the load multipliers at which the frame becomes unstable.

## Choosing A Surface

Both eigen searches share `EigOptions`. `modes` needs a density; `buckling` needs a compressive reference load.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Mass and modes of a cantilever.

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
### Example 2

Buckling of a pinned column.

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
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  const int members = 8;
  const auto column = beam(members, false);
  const double euler = M_PI * M_PI * 200e9 * 8.333e-8 / 4.0;
  out.writeln("Euler load of the pinned column (N): ", euler);

  // a 1000 N compressive reference load along the column: the factors times 1000 are the critical loads
  std::vector<double> load(3 * (members + 1), 0.0);
  load[3 * members] = -1000.0;
  structure::EigOptions options;
  options.count = 3;
  const auto r = structure::buckling(column, grid(load, 3), {{0, 0, members}, {0, 1, 1}, {}}, options);
  out.writeln("factors: ", flat(r.factor), ", critical loads (N): ", flat(r.factor.mul(1000.0)));
  out.writeln("converged ", flat(r.converged), ", residuals ", flat(r.residual), ", shape array ", r.shape.sizes(), ", iterations ", r.iterations);

  // the same column fixed at the base and free at the top (a cantilever): a quarter of the load
  const auto cantilever = structure::buckling(column, grid(load, 3), {{0, 0, 0}, {0, 1, 2}, {}});
  out.writeln("cantilever critical load (N): ", flat(cantilever.factor)[0] * 1000.0, "   Euler / 4 = ", euler / 4.0);

  // a reference load that doesn't compress anything
  std::vector<double> pull(3 * (members + 1), 0.0);
  pull[3 * members] = 1000.0;
  try {
    (void)structure::buckling(column, grid(pull, 3), {{0, 0, members}, {0, 1, 1}, {}});
  } catch (const std::exception &e) {
    std::cout << "tensile reference load: " << e.what() << "\n";
  }
}
```

Output

```text
Euler load of the pinned column (N): 41121.71
factors: [41.12, 164.57, 371.02], critical loads (N): [41123.05, 164571.07, 371020.06]
converged [1.00, 1.00, 1.00], residuals [0.00, 0.00, 0.00], shape array [3, 9, 3], iterations 25
cantilever critical load (N): 10280.45   Euler / 4 = 10280.43
tensile reference load: eng::structure::buckling: the reference load puts no member in compression, so there is no buckling factor; apply a compressive axial reference load (a zero, tensile or purely transverse load never buckles a frame in a linearised analysis)
```
### Example 3

Four end conditions against Euler's loads.

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
```{toctree}
:maxdepth: 2
:hidden:

spectral/EigOptions
spectral/ModeResult
spectral/BuckResult
spectral/mass
spectral/modes
spectral/buckling
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

spectral/EigOptions
spectral/ModeResult
spectral/BuckResult
spectral/mass
spectral/modes
spectral/buckling
```
<!-- apiref:toctree:end -->
