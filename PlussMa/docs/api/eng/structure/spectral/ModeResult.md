<!-- generated from ModeResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::ModeResult`

Natural frequencies and mode shapes of a frame.

## Signature

```cpp
structure::ModeResult
```

## What It Does

The result of `modes`: the lowest natural frequencies and their mass-normalised shapes.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `omega` | Angular frequencies, radians per time, ascending, `[..., mode]`. |  |
| `hertz` | The same frequencies in cycles per time, `[..., mode]`. |  |
| `shape` | Mode shapes `(ux, uy, rz)`, `[..., mode, node, 3]`, normalised so that `phi_i^T M phi_j` is `1` for `i = j` and `0` otherwise. The overall sign is arbitrary. |  |
| `residual` | `|| K phi - omega^2 M phi ||` of each pair, `[..., mode]`. |  |
| `converged` | One where the pair reached its tolerance, `[..., mode]`. |  |
| `iterations` | Iterations used. |  |

## Returns

Returned by `modes`.

## Errors

Reading a field does not solve again.

## Interpretation and Use

The frequencies belong to the constrained structure. The shape has no physical size, only a pattern, and its sign is arbitrary.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field of a modal solve.

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

Modes used to build a frequency response.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <complex>

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
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;
  const double force = 10.0, b = 1e-4;
  const std::vector<double> frequencies = {10, 30, 100, 150};
  std::vector<double> loads;
  for (std::size_t f = 0; f < frequencies.size(); ++f) {
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = force;
    loads.insert(loads.end(), load.begin(), load.end());
  }
  structure::HarmonicOptions options;
  options.stiffness_damping = b;
  const auto h = structure::harmonic(frame, pluss::series::make(loads).reshape({(std::int64_t)frequencies.size(), members + 1, 3}), vec(frequencies), wall, options);
  const auto direct = h.displacement.select(-2, members).select(-1, 1);
  const auto re = flat(direct.real()), im = flat(direct.imag());

  // the same response as a sum over the first six mass-normalised modes
  structure::EigOptions eig;
  eig.count = 6;
  const auto modes = structure::modes(frame, wall, eig);
  const auto omega = flat(modes.omega), shape = flat(modes.shape);
  out.writeln("omega    direct (mm, phase deg)       mode sum (mm, phase deg)");
  for (std::size_t f = 0; f < frequencies.size(); ++f) {
    std::complex<double> sum = 0.0;
    for (int n = 0; n < 6; ++n) {
      const double phi = shape[n * 3 * (members + 1) + tip];
      sum += phi * phi * force / std::complex<double>(omega[n] * omega[n] - frequencies[f] * frequencies[f], b * omega[n] * omega[n] * frequencies[f]);
    }
    out.writeln(frequencies[f], "   ", 1000.0 * std::hypot(re[f], im[f]), " ", std::atan2(im[f], re[f]) * 180.0 / M_PI, "      ", 1000.0 * std::abs(sum), " ", std::arg(sum) * 180.0 / M_PI);
  }
}
```

Output

```text
omega    direct (mm, phase deg)       mode sum (mm, phase deg)
10.0000   1.7010 -0.0609      1.7007 -0.0609
30.0000   3.4886 -0.3780      3.4883 -0.3780
100.0000   0.2505 -179.7192      0.2508 -179.7201
150.0000   0.0538 -178.2308      0.0541 -178.2438
```
### Example 3

Modes that start a damped free vibration.

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
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;
  const int steps = 200;
  const double dt = 0.001;
  std::vector<double> times, history;
  for (int i = 0; i <= steps; ++i) {
    times.push_back(dt * i);
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = -100.0;   // a suddenly applied 100 N at the tip
    history.insert(history.end(), load.begin(), load.end());
  }
  const auto load = pluss::series::make(history).reshape({steps + 1, members + 1, 3});

  const auto r = structure::motion(frame, load, vec(times), wall);
  const auto u = flat(r.displacement);
  double peak = 0.0;
  for (int i = 0; i <= steps; ++i) peak = std::min(peak, u[i * 3 * (members + 1) + tip]);
  out.writeln("sudden load, no damping: peak tip deflection (mm) ", 1000.0 * peak, " (static ", -1000.0 * 100.0 * 8.0 / (3 * 200e9 * 8.333e-8), "), every step converged ", r.converged.all().item<bool>());
  out.writeln("shapes: displacement ", r.displacement.sizes(), ", kinetic ", r.kinetic.sizes(), ", iterations per step ", r.iterations.sizes(), ", reaction at the wall at the last step ", flat(r.reaction.select(-3, steps).select(-2, 0))[1]);

  // initial state: released from the first mode shape with Rayleigh damping set for 2 percent at the first mode
  const auto modes = structure::modes(frame, wall);
  const double omega = flat(modes.omega)[0];
  structure::MotionState start;
  start.displacement = modes.shape.select(0, 0).mul(0.01);
  structure::MotionOptions options;
  options.stiffness_damping = 2.0 * 0.02 / omega;
  const auto none = pluss::series::zeros({steps + 1, members + 1, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto d = structure::motion(frame, none, vec(times), wall, start, options);
  const auto kinetic = flat(d.kinetic), strain = flat(d.strain_energy);
  out.writeln("damped free vibration: energy at the start and at 0.2 s (J) ", kinetic[0] + strain[0], " ", kinetic[steps] + strain[steps]);

  // mass damping and numerical damping
  structure::MotionOptions mass;
  mass.mass_damping = 2.0 * 0.02 * omega;
  const auto m = structure::motion(frame, none, vec(times), wall, start, mass);
  out.writeln("mass damping, same ratio at mode 1: energy at 0.2 s (J) ", flat(m.kinetic)[steps] + flat(m.strain_energy)[steps]);
  structure::MotionOptions numerical;
  numerical.spectral_radius = 0.5;
  const auto n = structure::motion(frame, load, vec(times), wall, {}, numerical);
  const auto damped = flat(n.displacement);
  double damped_peak = 0.0;
  for (int i = 0; i <= steps; ++i) damped_peak = std::min(damped_peak, damped[i * 3 * (members + 1) + tip]);
  out.writeln("spectral_radius 0.5: peak tip deflection (mm) ", 1000.0 * damped_peak);
}
```

Output

```text
sudden load, no damping: peak tip deflection (mm) -31.39733 (static -16.00064), every step converged true
shapes: displacement [201, 9, 3], kinetic [201], iterations per step [201], reaction at the wall at the last step 310.71429
damped free vibration: energy at the start and at 0.2 s (J) 0.08202 0.05882
mass damping, same ratio at mode 1: energy at 0.2 s (J) 0.05882
spectral_radius 0.5: peak tip deflection (mm) -31.39484
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="EigOptions.html">
    <span class="label">Previous</span>
    <span class="title">EigOptions</span>
  </a>
  <a class="next-link" href="BuckResult.html">
    <span class="label">Next</span>
    <span class="title">BuckResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
