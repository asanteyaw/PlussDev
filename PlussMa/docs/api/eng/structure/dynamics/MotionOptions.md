<!-- generated from MotionOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::MotionOptions`

Controls of the linear time integration.

## Signature

```cpp
structure::MotionOptions
```

## What It Does

Controls for the linear `motion`, which integrates `M a + C v + K u = f` with `C = mass_damping M + stiffness_damping K` by the generalised-alpha method.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `spectral_radius` | Response factor of the highest modes per step, in `[0, 1]`. `1` is the energy-conserving average-acceleration rule; smaller values add numerical damping of the poorly resolved high modes. | `1` |
| `mass_damping` | The coefficient `a` of `a M`; the damping ratio of mode `n` is `a / (2 omega_n)`, so it damps low modes most. | `0` |
| `stiffness_damping` | The coefficient `b` of `b K`; the damping ratio of mode `n` is `b omega_n / 2`, so it damps high modes most. | `0` |
| `solve` | Conjugate-gradient controls of the inner solves. | budget 8192 |
| `fail_on_nonconvergence` | When true a step whose solve does not converge throws; when false the history is returned with `converged` false at those steps. | `true` |

## Returns

A value; validated by `motion`.

## Errors

A `spectral_radius` outside `[0, 1]` or negative damping coefficients are rejected.

## Interpretation and Use

Set `a` and `b` for the damping ratio you want at the modes that matter; with both, the ratio is `(a / omega + b omega) / 2`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Rayleigh and numerical damping on a cantilever.

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
### Example 2

Rejected values.

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
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const auto times = vec({0.0, 0.001, 0.002});
  const auto nodal = grid(std::vector<double>(3 * (members + 1), 0.0), 3);
  const auto history = pluss::series::zeros({3, members + 1, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  report("motion, load without a time axis", [&] { (void)structure::motion(frame, nodal, times, wall); });
  report("motion, time not increasing     ", [&] { (void)structure::motion(frame, history, vec({0.0, 0.002, 0.001}), wall); });
  report("motion, frame without density   ", [&] { (void)structure::motion(beam(members, false), history, times, wall); });
  report("motion, spectral radius above 1 ", [&] { structure::MotionOptions o; o.spectral_radius = 1.5; (void)structure::motion(frame, history, times, wall, {}, o); });
  report("harmonic, load without frequency axis", [&] { (void)structure::harmonic(frame, nodal, vec({10.0}), wall); });
  report("harmonic, frame without density ", [&] { (void)structure::harmonic(beam(members, false), history.slice(0, 0, 1), vec({10.0}), wall); });
}
```

Output

```text
motion, load without a time axis: eng::structure::motion: load must be finite real floating [..., time, node, 3] with the frame dtype and platform
motion, time not increasing     : eng::structure::motion: time samples must be strictly increasing
motion, frame without density   : eng::structure::motion: frame sections do not define density
motion, spectral radius above 1 : eng::structure::motion: spectral_radius must be finite and in [0, 1]
harmonic, load without frequency axis: eng::structure::harmonic: load must be finite [..., frequency, node, 3] with the frame precision and platform
harmonic, frame without density : eng::structure::harmonic: frame sections do not define density
```
### Example 3

Damping in the frequency domain.

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
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;
  const std::vector<double> frequencies = {10, 40.5, 100, 253.8};
  std::vector<double> loads;
  for (std::size_t f = 0; f < frequencies.size(); ++f) {
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = 10.0;
    loads.insert(loads.end(), load.begin(), load.end());
  }
  const auto load = pluss::series::make(loads).reshape({(std::int64_t)frequencies.size(), members + 1, 3});

  structure::HarmonicOptions stiffness;
  stiffness.stiffness_damping = 1e-4;   // zeta_n = b omega_n / 2, 0.002 at the first mode
  const auto r = structure::harmonic(frame, load, vec(frequencies), wall, stiffness);
  out.writeln("stiffness damping: tip amplitude (mm) ", flat(r.displacement.select(-2, members).select(-1, 1).abs().mul(1000.0)), ", converged ", r.converged.all().item<bool>());
  out.writeln("shapes: displacement ", r.displacement.sizes(), ", reaction ", r.reaction.sizes(), ", relative residual ", flat(r.relative_residual));
  out.writeln("iterations ", r.iterations, ", operator evaluations ", r.operator_evaluations, ", restarts ", r.restarts);

  // mass damping: zeta_n = a / (2 omega_n), the same 0.002 at the first mode, but much heavier damping at low frequency and none at high frequency
  const double omega1 = flat(structure::modes(frame, wall).omega)[0];
  structure::HarmonicOptions mass;
  mass.mass_damping = 2.0 * 0.002 * omega1;
  const auto m = structure::harmonic(frame, load, vec(frequencies), wall, mass);
  out.writeln("mass damping:      tip amplitude (mm) ", flat(m.displacement.select(-2, members).select(-1, 1).abs().mul(1000.0)));

  // velocity and acceleration
  out.writeln("|velocity| / (omega |u|) at the tip: ", flat(r.velocity.select(-2, members).select(-1, 1).abs().div(r.displacement.select(-2, members).select(-1, 1).abs()).div(vec(frequencies))));

  // undamped at the natural frequency: no steady response
  structure::HarmonicOptions undamped;
  undamped.fail_on_nonconvergence = false;
  const auto u = structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1}), wall, undamped);
  out.writeln("undamped at omega_1, failure off: converged ", u.converged.all().item<bool>());
  try {
    undamped.fail_on_nonconvergence = true;
    (void)structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1}), wall, undamped);
  } catch (const std::exception &e) {
    std::cout << "undamped at omega_1, failure on: " << e.what() << "\n";
  }
}
```

Output

```text
stiffness damping: tip amplitude (mm) [1.7010, 383.4239, 0.2505, 1.5585], converged true
shapes: displacement [4, 9, 3], reaction [4, 9, 3], relative residual [0.0000, 0.0000, 0.0000, 0.0000]
iterations 24, operator evaluations 26, restarts 1
mass damping:      tip amplitude (mm) [1.7010, 388.2302, 0.2505, 55.7885]
|velocity| / (omega |u|) at the tip: [1.0000, 1.0000, 1.0000, 1.0000]
undamped at omega_1, failure off: converged false
undamped at omega_1, failure on: eng::structure::harmonic: dynamic stiffness solve did not converge; a sampled frequency may be singular or poorly conditioned
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="MotionState.html">
    <span class="label">Previous</span>
    <span class="title">MotionState</span>
  </a>
  <a class="next-link" href="NonlinearMotionOptions.html">
    <span class="label">Next</span>
    <span class="title">NonlinearMotionOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
