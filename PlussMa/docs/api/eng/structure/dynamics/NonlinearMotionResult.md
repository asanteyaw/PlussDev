<!-- generated from NonlinearMotionResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::NonlinearMotionResult`

The time history of a large-displacement integration.

## Signature

```cpp
structure::NonlinearMotionResult
```

## What It Does

The result of the nonlinear `motion`. Arrays have the leading batch axes, then the time axis.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `time` | The sample times, `[time]`. |  |
| `displacement` | Node displacements from the original positions, `[..., time, node, 3]`. |  |
| `velocity` | Node velocities, `[..., time, node, 3]`. |  |
| `acceleration` | Node accelerations, `[..., time, node, 3]`. |  |
| `reaction` | Support force and moment in global axes, `[..., time, node, 3]`. |  |
| `kinetic` | Kinetic energy, `[..., time]`. |  |
| `strain_energy` | Strain energy, `[..., time]`. |  |
| `residual_norm` | Out-of-balance force of each step. |  |
| `converged` | One where the step converged. |  |
| `iterations` | The largest Newton iteration count at each step, `[time]`. |  |

## Returns

Returned by the nonlinear `motion`.

## Errors

Reading a field does not integrate again.

## Interpretation and Use

Check `converged` when `fail_on_nonconvergence` is false.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field on a swinging rod.

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
stiffness_damping 1e-4: tip rotation at 0.6 s (rad) -0.56242
```
### Example 2

The linear history for comparison.

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
### Example 3

Rejected inputs.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="MotionResult.html">
    <span class="label">Previous</span>
    <span class="title">MotionResult</span>
  </a>
  <a class="next-link" href="HarmonicResult.html">
    <span class="label">Next</span>
    <span class="title">HarmonicResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
