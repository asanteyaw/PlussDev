<!-- generated from mass_and_modes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Mass and natural modes

[Back to Structures 3: Dynamics](../structure_03_dynamics.md)

A structure that is disturbed and then left alone vibrates in a combination of its **natural modes**: shapes that it can move in, each at its own frequency, with every part moving in step. The lowest few frequencies matter most. They decide whether a floor feels lively, whether a machine's running speed excites a resonance, and how the structure responds to a gust or an earthquake. `structure::modes` finds them. This page gives the frame a mass, checks the mass against the physical mass, finds the modes of a cantilever against the closed formulas, and shows how the mode shapes are scaled and how many to ask for.

## The mental model

Give each member a density, and the frame has a **mass** as well as a stiffness. Free vibration without damping is governed by

```{math}
K \phi = \omega^2 M \phi,
```

where `K` is the stiffness, `M` the mass, `omega` an angular frequency and `phi` a mode shape. `modes(frame, fix, options)` finds the lowest few `(omega, phi)` pairs of the structure with the constrained degrees of freedom held fixed. It returns a `ModeResult`:

| Member | Shape | Meaning |
|---|---|---|
| `omega` | `[..., mode]` | angular frequency in radians per second, ascending |
| `hertz` | `[..., mode]` | the same frequency in cycles per second, `omega / (2 pi)` |
| `shape` | `[..., mode, node, 3]` | the mode shape `(ux, uy, rz)` at every node, **mass-normalised** |
| `residual` | `[..., mode]` | `\|\| K phi - omega^2 M phi \|\|`, how well the pair solves the equation |
| `converged` | `[..., mode]` | one when the mode reached its tolerance |
| `iterations` | (a number) | iterations used |

A mode shape is defined only up to a factor, so the library fixes the factor: `phi^T M phi = 1`. The shapes are then **orthonormal** under the mass, `phi_i^T M phi_j` is one for `i = j` and zero otherwise, which is the form needed to combine modes into a response. The overall **sign** of a shape is arbitrary; do not rely on it.

`EigOptions` controls the search: `count` is how many modes to find (default 1), `max_iterations` and the two tolerances govern the iteration, and `solve` holds the conjugate-gradient settings of the inner stiffness solves.

## Checking the mass

Before trusting frequencies, check that the frame has the mass you intended. `structure::mass(frame, acceleration)` applies the frame's consistent mass matrix to an acceleration of every node, shape `[..., node, 3]`, and returns the inertial force it takes to produce it. A uniform acceleration of one in the y direction, with no rotation, takes a total force equal to the total mass, `rho A L`.

The program builds a cantilever `2 m` long with `A = 1e-3 m^2` and `rho = 7850 kg/m^3`, so the mass is `7850 * 1e-3 * 2 = 15.7 kg`, and prints the total force for unit accelerations in x and in y.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

// a straight steel beam along x with density, so it has mass
static st::Frame beam(int members, double length) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(length * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  const std::vector<double> area(members, 1e-3), inertia(members, 8.333e-8), modulus(members, 200e9), density(members, 7850.0);
  return st::frame({grid(position, 2), from, to}, {vec(area), vec(inertia), vec(modulus), vec(density)});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 8;
  const auto frame = beam(members, 2.0);

  for (int axis = 0; axis < 2; ++axis) {
    // a unit acceleration along one axis at every node
    std::vector<double> acceleration(3 * (members + 1), 0.0);
    for (int node = 0; node <= members; ++node) acceleration[3 * node + axis] = 1.0;
    const auto force = st::mass(frame, grid(acceleration, 3));
    double total = 0.0;
    for (int node = 0; node <= members; ++node) total += force.select(-2, node).select(-1, axis).item<double>();
    out.writeln("total inertial force for a unit acceleration along ", axis == 0 ? "x" : "y", " (kg): ", total);
  }
  out.writeln("physical mass rho A L (kg): ", 7850.0 * 1e-3 * 2.0);
}
```

Output

```text
total inertial force for a unit acceleration along x (kg): 15.7000
total inertial force for a unit acceleration along y (kg): 15.7000
physical mass rho A L (kg): 15.7000
```

Both totals equal the physical mass of `15.7 kg`. Without a density, `mass` and `modes` refuse to run and say so.

## The modes of a cantilever

A cantilever of length `L` has bending natural frequencies

```{math}
\omega_n = (\beta_n L)^2 \sqrt{\frac{E I}{\rho A L^4}}, \qquad \beta_n L = 1.8751,\; 4.6941,\; 7.8548,\; 10.9955,\ \ldots
```

For this beam the root is `sqrt(E I / (rho A L^4)) = 11.519`, so the first three are `40.50, 253.8, 710.7 rad/s`. The finite-element answer approaches these from above as the beam is divided into more members, because a stiffer-than-exact model is the price of fewer degrees of freedom. The program finds the lowest three modes for 2, 4, 8 and 16 members and prints the frequency error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static st::Frame beam(int members, double length) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(length * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  const std::vector<double> area(members, 1e-3), inertia(members, 8.333e-8), modulus(members, 200e9), density(members, 7850.0);
  return st::frame({grid(position, 2), from, to}, {vec(area), vec(inertia), vec(modulus), vec(density)});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const double length = 2.0, root = std::sqrt(200e9 * 8.333e-8 / (7850.0 * 1e-3 * std::pow(length, 4)));
  const double beta[3] = {1.875104, 4.694091, 7.854757};
  const st::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};

  out.writeln("exact (rad/s): ", beta[0] * beta[0] * root, " ", beta[1] * beta[1] * root, " ", beta[2] * beta[2] * root);
  for (int members : {2, 4, 8, 16}) {
    st::EigOptions options;
    options.count = 3;
    const auto modes = st::modes(beam(members, length), wall, options);
    const auto omega = flat(modes.omega);
    out.writeln(members, " members: ", omega, "   relative error (%): ", 100.0 * (omega[0] / (beta[0] * beta[0] * root) - 1.0), " ", 100.0 * (omega[1] / (beta[1] * beta[1] * root) - 1.0), " ",
                100.0 * (omega[2] / (beta[2] * beta[2] * root) - 1.0), "   all converged: ", modes.converged.all().item<bool>());
  }
}
```

Output

```text
exact (rad/s): 40.502 253.819 710.700
2 members: [40.521, 255.973, 865.747]   relative error (%): 0.048 0.849 21.816   all converged: true
4 members: [40.503, 254.115, 716.203]   relative error (%): 0.003 0.117 0.774   all converged: true
8 members: [40.502, 253.839, 711.132]   relative error (%): 0.000 0.008 0.061   all converged: true
16 members: [40.502, 253.820, 710.728]   relative error (%): 0.000 0.001 0.004   all converged: true
```

With eight members the first mode is accurate to about a thousandth of a percent; the higher modes need finer meshes because their shapes vary faster along the beam: the third mode is off by `0.06 percent` with eight members and by `22 percent` with two. Check the highest mode you use by halving the member length and seeing how far it moves.

## Orthonormal shapes and what a mode looks like

The program finds four modes of the eight-member cantilever, prints the matrix `phi_i^T M phi_j` that should be the identity, the residuals, and the transverse displacement `uy` of the first two modes at the nodes. The first mode has no node between the wall and the tip, and the second has one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static st::Frame beam(int members, double length) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(length * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  const std::vector<double> area(members, 1e-3), inertia(members, 8.333e-8), modulus(members, 200e9), density(members, 7850.0);
  return st::frame({grid(position, 2), from, to}, {vec(area), vec(inertia), vec(modulus), vec(density)});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 8;
  const auto frame = beam(members, 2.0);
  st::EigOptions options;
  options.count = 4;
  const auto modes = st::modes(frame, {{0, 0, 0}, {0, 1, 2}, {}}, options);

  out.writeln("residuals ||K phi - omega^2 M phi||: ", flat(modes.residual), "   converged: ", flat(modes.converged), "   iterations: ", modes.iterations);
  out.writeln("phi_i' M phi_j:");
  for (int i = 0; i < 4; ++i) {
    std::vector<double> row;
    for (int j = 0; j < 4; ++j) row.push_back(st::mass(frame, modes.shape.select(0, i)).mul(modes.shape.select(0, j)).sum().item<double>() + 0.0);
    out.writeln("  ", row);
  }

  // the first two shapes: (uy, rz) at every node, scaled so that the tip of each is positive
  for (int mode = 0; mode < 2; ++mode) {
    auto shape = modes.shape.select(0, mode);
    const double sign = shape.select(-2, members).select(-1, 1).item<double>() < 0 ? -1.0 : 1.0;
    out.writeln("mode ", mode + 1, " uy along the beam: ", flat(shape.select(-1, 1).mul(sign)));
  }
}
```

Output

```text
residuals ||K phi - omega^2 M phi||: [0.0000, 0.0000, 0.0002, 0.0145]   converged: [1.0000, 1.0000, 1.0000, 1.0000]   iterations: 19
phi_i' M phi_j:
  [1.0000, -0.0000, -0.0000, 0.0000]
  [-0.0000, 1.0000, -0.0000, -0.0000]
  [-0.0000, -0.0000, 1.0000, 0.0000]
  [-0.0000, -0.0000, 0.0000, 1.0000]
mode 1 uy along the beam: [-0.0000, 0.0131, 0.0491, 0.1034, 0.1714, 0.2488, 0.3320, 0.4180, 0.5048]
mode 2 uy along the beam: [-0.0000, -0.0696, -0.2106, -0.3303, -0.3603, -0.2698, -0.0681, 0.2053, 0.5048]
```

The matrix is the identity to the printed digits, the residuals are tiny beside the terms of the equation (the stiffness entries are of order `1e8`), and the first mode's `uy` grows steadily from the wall to the tip, while the second changes sign once between them. The sign flip applied to each shape here is only for display: the library's own sign is arbitrary.

## Tolerance and iteration limits

`EigOptions` also carries the convergence controls of the search. `relative_tolerance` and `absolute_tolerance` say how close each pair must be before it is accepted, and `max_iterations` caps the work. The program finds the three lowest modes of the eight-member cantilever with the defaults, with a loose relative tolerance of `1e-3`, and with a budget of only two iterations, and prints the frequencies, the residuals and whether each mode converged.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static st::Frame beam(int members) {
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
  return st::frame({grid(position, 2), from, to},
                   {vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9)), vec(std::vector<double>(members, 7850.0))});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto frame = beam(8);
  const st::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};

  st::EigOptions defaults;
  defaults.count = 3;
  st::EigOptions loose = defaults;
  loose.relative_tolerance = 1e-3;
  st::EigOptions short_budget = defaults;
  short_budget.max_iterations = 2;
  const struct { const char *name; st::EigOptions options; } runs[] = {{"defaults             ", defaults}, {"relative tolerance 1e-3", loose}, {"max_iterations 2     ", short_budget}};
  for (const auto &run : runs) {
    const auto r = st::modes(frame, wall, run.options);
    out.writeln(run.name, ": omega ", flat(r.omega), ", residual ", flat(r.residual), ", converged ", flat(r.converged), ", iterations ", r.iterations);
  }
}
```

Output

```text
defaults             : omega [40.5016, 253.8392, 711.1325], residual [0.0000, 0.0000, 0.0025], converged [1.0000, 1.0000, 1.0000], iterations 15
relative tolerance 1e-3: omega [40.5016, 253.8392, 711.1325], residual [0.0000, 0.0015, 466.7120], converged [1.0000, 1.0000, 1.0000], iterations 6
max_iterations 2     : omega [40.5016, 253.8404, 714.7590], residual [0.7465, 1269.0620, 105059.3050], converged [0.0000, 0.0000, 0.0000], iterations 2
```

A loose tolerance still gives the frequencies to four digits here, with fewer iterations, but leaves larger residuals; a budget of two iterations stops before any mode converges, and `converged` says so for every mode. Check `converged` whenever you reduce the budget.

## Axial modes among the bending modes

A beam also has modes in which it stretches and shortens along its length. They are far stiffer than bending, so they sit at much higher frequencies, but they are in the list. A cantilever of length `L` has axial frequencies `omega = (2 n - 1) (pi / 2) sqrt(E / rho) / L`, the first being `(pi / 2) * 5047.5 / 2 = 3964.3 rad/s`. The program asks for ten modes of a sixteen-member cantilever and measures how much of each mode's motion is along the beam, the share of `ux`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(1).with_sep(""));
  const int members = 16;
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
  const auto frame = st::frame({grid(position, 2), from, to},
                               {vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9)), vec(std::vector<double>(members, 7850.0))});
  st::EigOptions options;
  options.count = 10;
  const auto modes = st::modes(frame, {{0, 0, 0}, {0, 1, 2}, {}}, options);
  const auto omega = flat(modes.omega);
  for (int mode = 0; mode < 10; ++mode) {
    const auto shape = modes.shape.select(0, mode);
    const double along = shape.select(-1, 0).square().sum().item<double>();
    const double across = shape.select(-1, 1).square().sum().item<double>();
    out.writeln("mode ", mode + 1, ": omega ", omega[mode], " rad/s, share of the motion along the beam ", along / (along + across));
  }
  out.writeln("first axial frequency from the formula: ", 0.5 * M_PI * std::sqrt(200e9 / 7850.0) / 2.0, " rad/s");
}
```

Output

```text
mode 1: omega 40.5 rad/s, share of the motion along the beam 0.0
mode 2: omega 253.8 rad/s, share of the motion along the beam 0.0
mode 3: omega 710.7 rad/s, share of the motion along the beam 0.0
mode 4: omega 1392.9 rad/s, share of the motion along the beam 0.0
mode 5: omega 2303.2 rad/s, share of the motion along the beam 0.0
mode 6: omega 3442.2 rad/s, share of the motion along the beam 0.0
mode 7: omega 3965.9 rad/s, share of the motion along the beam 1.0
mode 8: omega 4811.7 rad/s, share of the motion along the beam 0.0
mode 9: omega 6414.2 rad/s, share of the motion along the beam 0.0
mode 10: omega 8253.8 rad/s, share of the motion along the beam 0.0
first axial frequency from the formula: 3964.3 rad/s
```

Mode 7 is the stretching mode: its motion is entirely along the beam and its frequency, `3965.9 rad/s`, is within `0.05 percent` of the formula, while the other nine are bending modes. If a mode does not look like the bending you expect, check how much of it is axial before interpreting it.

## Decisions

- **How many modes:** enough that the highest one you use lies well above the highest frequency in the loading you care about, and never more than the mesh resolves.
- **Frequency in hertz or in rad/s:** `omega` for the equations, `hertz` for comparing with machine speeds and people's perception.
- **A mesh check:** halve the member length and re-solve; a mode that moves by more than a fraction of a percent needs a finer mesh.

## Pitfalls

The program makes three mistakes whose results look reasonable: expecting the frequencies of a cantilever from a beam that is fixed at both ends, running `modes` on a frame without a density, and reading a mode shape as a displacement of a particular size.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a straight steel beam along x, 2 m long, with density
static st::Frame beam(int members, bool with_density = true) {
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
  st::Sections sections{vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))};
  if (with_density) sections.density = vec(std::vector<double>(members, 7850.0));
  return st::frame({grid(position, 2), from, to}, sections);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  const int members = 8;
  const auto frame = beam(members);
  st::EigOptions options;
  options.count = 2;

  // 1. the frequencies belong to the constrained structure
  const auto cantilever = st::modes(frame, {{0, 0, 0}, {0, 1, 2}, {}}, options);
  const auto clamped = st::modes(frame, {{0, 0, 0, members, members, members}, {0, 1, 2, 0, 1, 2}, {}}, options);
  out.writeln("fixed at one end:   first two frequencies (rad/s) ", flat(cantilever.omega));
  out.writeln("fixed at both ends: first two frequencies (rad/s) ", flat(clamped.omega));

  // 2. a frame without a density has no mass
  try {
    (void)st::modes(beam(members, false), {{0, 0, 0}, {0, 1, 2}, {}}, options);
  } catch (const std::exception &e) {
    std::cout << "no density: " << e.what() << "\n";
  }

  // 3. a shape has no size of its own: it is scaled so that phi' M phi = 1, and a mode is only the pattern
  const auto shape = cantilever.shape.select(0, 0);
  out.writeln("first mode, tip uy and tip rotation as the library scales it: ", shape.select(-2, members).select(-1, 1).item<double>(), " ", shape.select(-2, members).select(-1, 2).item<double>(), "   (not displacements of a real motion)");
}
```

Output

```text
fixed at one end:   first two frequencies (rad/s) [40.50, 253.84]
fixed at both ends: first two frequencies (rad/s) [257.74, 710.87]
no density: eng::structure::modes: frame sections do not define density
first mode, tip uy and tip rotation as the library scales it: -0.50 -0.35   (not displacements of a real motion)
```

The frequencies of the beam fixed at both ends are more than six times the first frequency of the cantilever, since every constraint changes every mode; a missing density is refused with an error that names it; and the numbers in a mode shape, here `0.50` at the tip, come from the scaling `phi' M phi = 1` and carry meaning only relative to one another. Combine them with a response only through the mode's participation, as the [harmonic page](harmonic_response.md) does.

Reference: [`mass`](../../../../api/eng/structure/spectral/mass.md), [`modes`](../../../../api/eng/structure/spectral/modes.md), [`EigOptions`](../../../../api/eng/structure/spectral/EigOptions.md), [`ModeResult`](../../../../api/eng/structure/spectral/ModeResult.md), [`Frame`](../../../../api/eng/structure/model/FrameModel.md), [`Fix`](../../../../api/eng/structure/model/Fix.md).

Where next: [Buckling](buckling.md) finds the load at which a compressed member becomes unstable, with the same machinery.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="buckling.html">
    <span class="label">Next</span>
    <span class="title">Buckling</span>
  </a>
</div>
<!-- apiref:nav:end -->
