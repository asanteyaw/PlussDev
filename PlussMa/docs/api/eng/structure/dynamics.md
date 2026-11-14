<!-- generated from dynamics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Time and frequency response

This branch follows a frame through time and across frequency. `motion` integrates a load history (linear or with large rotations), and `harmonic` finds the steady response to sinusoidal loads at many frequencies in one call.

## Choosing A Surface

Use `harmonic` for a steady sinusoidal load and `motion` for loads that start, stop or change shape.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

A sudden load, damping and free vibration.

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

A rod swinging through a large angle.

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
  // a 1 m steel rod hanging from a pin at its top node, released at rest at 1 rad
  const int members = 8;
  const double gravity = 9.81, theta0 = 1.0;
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(0.0);
    position.push_back(-1.0 * i / members);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  const auto rod = structure::frame({grid(position, 2), from, to}, {vec(std::vector<double>(members, 4e-4)), vec(std::vector<double>(members, 1.333e-8)), vec(std::vector<double>(members, 200e9)), vec(std::vector<double>(members, 7850.0))});
  const structure::Fix pin{{0, 0}, {0, 1}, {}};
  std::vector<double> down(3 * (members + 1), 0.0);
  for (int i = 0; i <= members; ++i) down[3 * i + 1] = -gravity;
  const auto weight = flat(structure::mass(rod, grid(down, 3)));

  const int steps = 120;
  const double dt = 0.005;
  std::vector<double> times, history;
  for (int i = 0; i <= steps; ++i) {
    times.push_back(dt * i);
    history.insert(history.end(), weight.begin(), weight.end());
  }
  std::vector<double> start(3 * (members + 1), 0.0);
  for (int i = 0; i <= members; ++i) {
    const double x = 1.0 * i / members;
    start[3 * i] = x * std::sin(theta0);
    start[3 * i + 1] = x * (1.0 - std::cos(theta0));
    start[3 * i + 2] = theta0;
  }
  structure::MotionState initial;
  initial.displacement = grid(start, 3);

  structure::NonlinearMotionOptions options;   // the default spectral radius is 0.9
  const auto r = structure::motion(rod, pluss::series::make(history).reshape({steps + 1, members + 1, 3}), vec(times), pin, initial, options);
  out.writeln("default spectral radius ", options.spectral_radius, ": every step converged ", r.converged.all().item<bool>(), ", at most ", r.iterations.max().item<double>(), " Newton iterations per step");
  out.writeln("tip rotation at 0, 0.3 and 0.6 s (rad): ", flat(r.displacement.select(-2, members).select(-1, 2))[0], " ", flat(r.displacement.select(-2, members).select(-1, 2))[60], " ", flat(r.displacement.select(-2, members).select(-1, 2))[120]);
  out.writeln("shapes: displacement ", r.displacement.sizes(), ", kinetic ", r.kinetic.sizes());

  // spectral_radius 0.5 damps the high modes more
  structure::NonlinearMotionOptions damped;
  damped.spectral_radius = 0.5;
  const auto d = structure::motion(rod, pluss::series::make(history).reshape({steps + 1, members + 1, 3}), vec(times), pin, initial, damped);
  out.writeln("spectral_radius 0.5: tip rotation at 0.6 s (rad) ", flat(d.displacement.select(-2, members).select(-1, 2))[120]);

  // with mass and stiffness damping as well
  structure::NonlinearMotionOptions rayleigh;
  rayleigh.stiffness_damping = 1e-4;
  const auto q = structure::motion(rod, pluss::series::make(history).reshape({steps + 1, members + 1, 3}), vec(times), pin, initial, rayleigh);
  out.writeln("stiffness_damping 1e-4: tip rotation at 0.6 s (rad) ", flat(q.displacement.select(-2, members).select(-1, 2))[120]);
}
```

Output

```text
default spectral radius 0.90000: every step converged true, at most 3.00000 Newton iterations per step
tip rotation at 0, 0.3 and 0.6 s (rad): 1.00000 0.48006 -0.56304
shapes: displacement [121, 9, 3], kinetic [121]
spectral_radius 0.5: tip rotation at 0.6 s (rad) -0.56293
stiffness_damping 1e-4: tip rotation at 0.6 s (rad) 0.99901
```
### Example 3

A frequency sweep.

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

  // undamped: the amplitude grows without bound as the frequency approaches the natural frequency
  const std::vector<std::pair<const char *, double>> offsets = {{"1e-2", 1e-2}, {"1e-3", 1e-3}};
  for (const auto &offset : offsets) {
    const auto u = structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1 * (1.0 + offset.second)}), wall);
    out.writeln("no damping, ", offset.first, " above omega_1: tip amplitude (mm) ", 1000.0 * flat(u.displacement.select(-2, members).select(-1, 1).abs())[0]);
  }

  // exactly at the natural frequency the dynamic stiffness is singular: the solve reports that it did not converge
  structure::HarmonicOptions undamped;
  undamped.fail_on_nonconvergence = false;
  const auto exact = structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1}), wall, undamped);
  out.writeln("undamped at omega_1, failure off: converged ", exact.converged.all().item<bool>());
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
no damping, 1e-2 above omega_1: tip amplitude (mm) 77.2242
no damping, 1e-3 above omega_1: tip amplitude (mm) 776.1488
undamped at omega_1, failure off: converged false
undamped at omega_1, failure on: eng::structure::harmonic: dynamic stiffness solve did not converge; a sampled frequency may be singular or poorly conditioned
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

dynamics/MotionState
dynamics/MotionOptions
dynamics/NonlinearMotionOptions
dynamics/HarmonicOptions
dynamics/MotionResult
dynamics/NonlinearMotionResult
dynamics/HarmonicResult
dynamics/motion
dynamics/harmonic
```
<!-- apiref:toctree:end -->
