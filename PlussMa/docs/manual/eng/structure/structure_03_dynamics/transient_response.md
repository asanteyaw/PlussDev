<!-- generated from transient_response.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Transient response

[Back to Structures 3: Dynamics](../structure_03_dynamics.md)

A load that changes with time, a footfall, a gust, a shock or a machine switching on, sets a structure moving, and what matters is how far, how fast and for how long. `structure::motion` steps a frame through time under a load history. This page applies a sudden load and checks the peak against the known doubling, releases a beam from a given shape and checks the decay against the damping that was asked for, shows what the numerical damping control does, and drives a beam at its resonance and watches the response build up.

## The mental model

`motion(frame, load, time, fix, initial, options)` integrates

```{math}
M \ddot u + C \dot u + K u = f(t), \qquad C = a M + b K,
```

where `a` is the **mass damping** and `b` the **stiffness damping** (together, Rayleigh damping), `f(t)` the load history and `u` the node displacements with the constrained degrees of freedom held at zero. The inputs are:

| Input | Shape | Meaning |
|---|---|---|
| `load` | `[..., time, node, 3]` | the load at every node at every sample time |
| `time` | `[time]` | strictly increasing sample times, at least two |
| `initial` | `MotionState` | optional starting `displacement` and `velocity`, each `[..., node, 3]`; zero if empty |
| `options` | `MotionOptions` | `mass_damping`, `stiffness_damping`, `spectral_radius`, the conjugate-gradient settings in `solve`, and `fail_on_nonconvergence` |

The frame needs a density. The result is a `MotionResult`, with a leading batch shape and then time:

| Member | Shape | Meaning |
|---|---|---|
| `time` | `[time]` | the sample times |
| `displacement`, `velocity`, `acceleration` | `[..., time, node, 3]` | the motion of every node |
| `reaction` | `[..., time, node, 3]` | the support force and moment; zero at free degrees of freedom |
| `kinetic`, `strain_energy` | `[..., time]` | the two energies at every time |
| `residual_norm`, `converged`, `breakdown` | `[..., time]` | solve diagnostics at every step |
| `iterations` | `[time]` | the largest iteration count among the structures at each step |

The integrator is the generalised-alpha method, second-order accurate. With its default `spectral_radius` of one it is the average-acceleration rule: unconditionally stable, free of numerical damping, and conserving energy exactly for an undamped linear structure. The step between the samples of `time` is the step of the integration, so the samples must be fine enough to resolve the motion: the period error is about `(omega * dt)^2 / 12` of a period, so a step of one tenth of the shortest period you care about keeps that below one percent.

The damping parameters set a damping ratio for each mode, `zeta_n = (a / omega_n + b * omega_n) / 2`. Mass damping `a` damps the low modes most, stiffness damping `b` the high modes. Pick one or both to give the ratio you want at the modes that matter.

## A suddenly applied load

A load that jumps from zero to `P` and stays there makes a structure overshoot: it swings through its static deflection to nearly twice that value before settling. For a single mode the peak is exactly twice the static value, at half a period.

The program applies a `100 N` load pointing down at the tip of the eight-member cantilever, `2 m` long, `E I = 16.67 kN m^2`, from time zero, with no damping. It integrates for `0.2 s` in steps of `1 ms`, compares the peak with the static deflection from `solve`, and compares the time of the peak with half the first period, `pi / omega_1`.

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
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const int members = 8;
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
  const st::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;   // index of the tip's y displacement in a flattened [node, 3] array

  // the static deflection under the same load
  std::vector<double> load(3 * (members + 1), 0.0);
  load[tip] = -100.0;
  const double static_tip = flat(st::solve(frame, grid(load, 3), wall).displacement)[tip];

  // the load held constant over 200 samples, 1 ms apart
  const int steps = 200;
  const double dt = 0.001;
  std::vector<double> times, history;
  for (int i = 0; i <= steps; ++i) {
    times.push_back(dt * i);
    history.insert(history.end(), load.begin(), load.end());
  }
  const auto r = st::motion(frame, pluss::series::make(history).reshape({steps + 1, members + 1, 3}), vec(times), wall);

  const auto u = flat(r.displacement);
  double peak = 0.0, when = 0.0;
  for (int i = 0; i <= steps; ++i)
    if (u[i * 3 * (members + 1) + tip] < peak) {
      peak = u[i * 3 * (members + 1) + tip];
      when = times[i];
    }
  st::EigOptions options;
  const double omega = flat(st::modes(frame, wall, options).omega)[0];
  out.writeln("static tip deflection (mm): ", 1000.0 * static_tip);
  out.writeln("peak tip deflection (mm): ", 1000.0 * peak, "   ratio to static: ", peak / static_tip);
  out.writeln("time of the peak (s): ", when, "   half the first period pi / omega_1: ", M_PI / omega);
  out.writeln("every step converged: ", r.converged.all().item<bool>());
}
```

Output

```text
static tip deflection (mm): -16.00064
peak tip deflection (mm): -31.39733   ratio to static: 1.96225
time of the peak (s): 0.08000   half the first period pi / omega_1: 0.07757
every step converged: true
```

The peak is `1.96` times the static deflection and occurs at `0.080 s`, within a step or two of half the first period, `0.0776 s` (the top of the peak is flat). It is a little below two because higher modes join the first, and their peaks do not all fall at the same instant. Dynamic load factors near two are the reason a suddenly applied load must be designed for twice the static effect.

## Free vibration and damping

Release the beam from its first mode shape with no load, and it oscillates at the first frequency, with an amplitude that decays at a rate set by the damping. In the program the shape is scaled to a small amplitude, applied through `MotionState`, and run with no damping and then with each kind of Rayleigh damping set for a damping ratio of `zeta = 0.02` at the first mode: `b = 2 zeta / omega_1` for stiffness damping, `a = 2 zeta omega_1` for mass damping. The amplitude after `k` damped periods, `T_d = 2 pi / (omega_1 sqrt(1 - zeta^2))`, should be `exp(-zeta omega_1 k T_d)` of the start.

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
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const int members = 8;
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
  const st::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;

  const auto modes = st::modes(frame, wall);
  const double omega = flat(modes.omega)[0], zeta = 0.02;
  st::MotionState start;
  start.displacement = modes.shape.select(0, 0).mul(0.01);   // the first mode shape, scaled small

  const int steps = 800;
  const double dt = 0.0005;
  std::vector<double> times;
  for (int i = 0; i <= steps; ++i) times.push_back(dt * i);
  const auto no_load = pluss::series::zeros({steps + 1, members + 1, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));

  const double damped_period = 2.0 * M_PI / (omega * std::sqrt(1.0 - zeta * zeta));
  struct Run { const char *name; double mass, stiffness; };
  for (const Run &run : {Run{"no damping       ", 0.0, 0.0}, Run{"stiffness damping", 0.0, 2.0 * zeta / omega}, Run{"mass damping     ", 2.0 * zeta * omega, 0.0}}) {
    st::MotionOptions options;
    options.mass_damping = run.mass;
    options.stiffness_damping = run.stiffness;
    const auto r = st::motion(frame, no_load, vec(times), wall, start, options);
    const auto u = flat(r.displacement);
    const auto kinetic = flat(r.kinetic), strain = flat(r.strain_energy);
    const double energy_start = kinetic[0] + strain[0], energy_end = kinetic[steps] + strain[steps];
    out.writeln(run.name, ": amplitude after 0, 1, 2 damped periods, relative to the start: ", 1.0, " ", u[std::lround(damped_period / dt) * 3 * (members + 1) + tip] / u[tip], " ",
                u[std::lround(2 * damped_period / dt) * 3 * (members + 1) + tip] / u[tip]);
    out.writeln("                    energy at the end over energy at the start: ", energy_end / energy_start);
  }
  out.writeln("expected amplitude ratios exp(-zeta omega k T_d), k = 1, 2: ", std::exp(-zeta * omega * damped_period), " ", std::exp(-zeta * omega * 2 * damped_period));
}
```

Output

```text
no damping       : amplitude after 0, 1, 2 damped periods, relative to the start: 1.00000 0.99998 0.99996
                    energy at the end over energy at the start: 1.00000
stiffness damping: amplitude after 0, 1, 2 damped periods, relative to the start: 1.00000 0.88188 0.77773
                    energy at the end over energy at the start: 0.53188
mass damping     : amplitude after 0, 1, 2 damped periods, relative to the start: 1.00000 0.88188 0.77773
                    energy at the end over energy at the start: 0.53188
expected amplitude ratios exp(-zeta omega k T_d), k = 1, 2: 0.88189 0.77773
```

Both kinds of damping give the amplitude decay the formula predicts (`0.88188` and `0.77773` against `0.88189` and `0.77773`), and identical decay, since only the first mode is excited and both were set for the same ratio there. Without damping the energy is conserved to the printed digits, and the amplitude after a period, `0.99998`, differs from one only by the integrator's tiny period error. The kinetic and strain energies are available at every time, so a conservation check of an undamped run is one line.

## Numerical damping

A step load excites many modes, and the high ones, which the mesh barely resolves, ring at frequencies that mean nothing physically. The average-acceleration rule keeps them ringing forever. Setting `spectral_radius` below one makes the integrator damp the highest-frequency content, leaving the low modes almost untouched. The value is the response factor of the highest modes per step: one gives no damping, and `0.5` damps strongly.

The program applies the step load of the first program for one second with `spectral_radius` of `1`, `0.9` and `0.5` and prints the peak tip deflection, the quantity of interest, together with the largest tip acceleration in the last fifth of the run, which is dominated by the ringing of the high modes.

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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 8;
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
  const st::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;

  const int steps = 1000;
  const double dt = 0.001;
  std::vector<double> times, history;
  for (int i = 0; i <= steps; ++i) {
    times.push_back(dt * i);
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = -1000.0;
    history.insert(history.end(), load.begin(), load.end());
  }
  const auto load = pluss::series::make(history).reshape({steps + 1, members + 1, 3});

  for (double radius : {1.0, 0.9, 0.5}) {
    st::MotionOptions options;
    options.spectral_radius = radius;
    const auto r = st::motion(frame, load, vec(times), wall, {}, options);
    const auto u = flat(r.displacement), a = flat(r.acceleration);
    double peak = 0.0, ringing = 0.0;
    for (int i = 0; i <= steps; ++i) {
      peak = std::max(peak, std::abs(u[i * 3 * (members + 1) + tip]));
      if (i >= 800) ringing = std::max(ringing, std::abs(a[i * 3 * (members + 1) + tip]));
    }
    out.writeln("spectral radius ", radius, ": peak tip deflection (m) ", peak, ", late largest tip acceleration (m/s^2) ", ringing);
  }
}
```

Output

```text
spectral radius 1.0000: peak tip deflection (m) 0.3190, late largest tip acceleration (m/s^2) 4758.5852
spectral radius 0.9000: peak tip deflection (m) 0.3189, late largest tip acceleration (m/s^2) 922.9535
spectral radius 0.5000: peak tip deflection (m) 0.3191, late largest tip acceleration (m/s^2) 475.9515
```

The peak deflection is the same to three digits in every run, while the high-frequency acceleration falls by a factor of five and then ten. Use `spectral_radius` below one when accelerations or reactions are what you are after and the ringing of poorly resolved modes would swamp them; leave it at one when conserving energy matters, or when the high modes are the point.

## A load at the resonance

A sinusoidal load at the first natural frequency pumps energy into the first mode, and the amplitude grows until damping takes out as much energy per cycle as the load puts in. With damping ratio `zeta`, the envelope grows as `1 - exp(-zeta omega_1 t)` toward a steady value. The program drives the tip with `100 N` at `omega_1`, with stiffness damping for `zeta = 0.05`, and prints the largest amplitude reached within the first 1, 2, 4, 8 and 16 cycles beside the envelope, scaled to the steady amplitude. That steady amplitude is the one that [harmonic response](harmonic_response.md) computes directly, without any time stepping, and the program prints it too for comparison.

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
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const int members = 8;
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
  const st::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;

  const double omega = flat(st::modes(frame, wall).omega)[0], zeta = 0.05, stiffness_damping = 2.0 * zeta / omega;
  const int steps = 4000;
  const double dt = 0.001;
  std::vector<double> times, history;
  for (int i = 0; i <= steps; ++i) {
    times.push_back(dt * i);
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = 100.0 * std::sin(omega * dt * i);
    history.insert(history.end(), load.begin(), load.end());
  }
  st::MotionOptions options;
  options.stiffness_damping = stiffness_damping;
  const auto r = st::motion(frame, pluss::series::make(history).reshape({steps + 1, members + 1, 3}), vec(times), wall, {}, options);
  const auto u = flat(r.displacement);

  // the steady amplitude, from the frequency-domain solve
  std::vector<double> unit(3 * (members + 1), 0.0);
  unit[tip] = 100.0;
  st::HarmonicOptions harmonic_options;
  harmonic_options.stiffness_damping = stiffness_damping;
  const auto h = st::harmonic(frame, grid(unit, 3).reshape({1, members + 1, 3}), vec({omega}), wall, harmonic_options);
  const double steady = flat(h.displacement.select(-2, members).select(-1, 1).abs())[0];
  out.writeln("steady amplitude from the harmonic solve (mm): ", 1000.0 * steady);

  for (int cycles : {1, 2, 4, 8, 16}) {
    double largest = 0.0;
    const int last = static_cast<int>(std::lround(cycles * 2.0 * M_PI / omega / dt));
    for (int i = 0; i <= last; ++i) largest = std::max(largest, std::abs(u[i * 3 * (members + 1) + tip]));
    out.writeln("within ", cycles, " cycle(s): largest tip amplitude (mm) ", 1000.0 * largest, "   envelope ", 1000.0 * steady * (1.0 - std::exp(-zeta * omega * cycles * 2.0 * M_PI / omega)));
  }
}
```

Output

```text
steady amplitude from the harmonic solve (mm): 155.36621
within 1 cycle(s): largest tip amplitude (mm) 41.97547   envelope 41.88631
within 2 cycle(s): largest tip amplitude (mm) 72.57723   envelope 72.48019
within 4 cycle(s): largest tip amplitude (mm) 111.24123   envelope 111.14750
within 8 cycle(s): largest tip amplitude (mm) 142.82996   envelope 142.78114
within 16 cycle(s): largest tip amplitude (mm) 154.33789   envelope 154.34679
```

The time-stepped amplitude follows the envelope closely and levels at the steady value from the frequency-domain solve, `155.4 mm`. A `100 N` load applied slowly gives a static tip deflection of `16.0 mm`, so the resonant response is `9.7` times larger, close to the single-mode amplification `1 / (2 zeta) = 10`. Damping is what limits it: with half the damping the steady amplitude doubles.

## Decisions

- **How fine a step:** at most one tenth of the shortest period of any mode you need, and fine enough that the load history is resolved; check by halving the step and seeing whether the peaks move.
- **Damping:** give a ratio to the modes that matter through `a` and `b`; if two modes are the ones of interest, solve `zeta_n = (a / omega_n + b omega_n) / 2` for the two modes.
- **Numerical damping:** `spectral_radius` of `0.9` for a step or impact load when accelerations or reactions matter; `1` otherwise.
- **Frequency or time domain:** a steady sinusoidal load is cheaper and cleaner in the frequency domain; use time stepping for loads that start, stop, pulse or change shape.

## Pitfalls

The program makes three mistakes whose results look like a motion: a constant load given without its time axis, a time step too coarse for a short pulse, and expecting stiffness damping calibrated at the first mode to hold at the higher ones.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 8;
  const auto frame = beam(members);
  const st::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;

  // 1. a constant load needs a time axis
  std::vector<double> load(3 * (members + 1), 0.0);
  load[tip] = -100.0;
  try {
    (void)st::motion(frame, grid(load, 3), vec({0.0, 0.001, 0.002}), wall);
  } catch (const std::exception &e) {
    std::cout << "load without a time axis: " << e.what() << "\n";
  }

  // 2. a 10 ms half-sine pulse of 1 kN: resolved by a 1 ms step, missed by a 100 ms step
  for (double dt : {0.001, 0.1}) {
    const int steps = static_cast<int>(std::lround(0.5 / dt));
    std::vector<double> times, history;
    for (int i = 0; i <= steps; ++i) {
      times.push_back(dt * i);
      std::vector<double> sample(3 * (members + 1), 0.0);
      sample[tip] = times.back() <= 0.01 ? -1000.0 * std::sin(M_PI * times.back() / 0.01) : 0.0;
      history.insert(history.end(), sample.begin(), sample.end());
    }
    const auto r = st::motion(frame, pluss::series::make(history).reshape({steps + 1, members + 1, 3}), vec(times), wall);
    const auto u = flat(r.displacement);
    double peak = 0.0;
    for (int i = 0; i <= steps; ++i) peak = std::max(peak, std::abs(u[i * 3 * (members + 1) + tip]));
    out.writeln("pulse sampled every ", dt, " s: peak tip deflection (mm) ", 1000.0 * peak);
  }

  // 3. stiffness damping gives a damping ratio that grows with frequency: zeta_n = b omega_n / 2
  st::EigOptions options;
  options.count = 5;
  const auto omega = flat(st::modes(frame, wall, options).omega);
  const double b = 2.0 * 0.02 / omega[0];
  std::vector<double> ratio;
  for (double w : omega) ratio.push_back(b * w / 2.0);
  out.writeln("stiffness damping set for 2 percent at mode 1: damping ratios of the first five modes ", ratio);
}
```

Output

```text
load without a time axis: eng::structure::motion: load must be finite real floating [..., time, node, 3] with the frame dtype and platform
pulse sampled every 0.0010 s: peak tip deflection (mm) 45.2547
pulse sampled every 0.1000 s: peak tip deflection (mm) 0.0000
stiffness damping set for 2 percent at mode 1: damping ratios of the first five modes [0.0200, 0.1253, 0.3512, 0.6893, 1.1434]
```

A `[node, 3]` load is rejected with an error that names the missing time axis. The `10 ms` pulse is resolved by a `1 ms` step; sampled every `100 ms` no sample falls inside it, the load seen by the solver is zero and the peak is zero: the pulse has vanished with no warning. And damping that gives two percent at the first mode gives `12.5 percent` at the second and `114 percent`, beyond critical, at the fifth: choose `a` and `b` for the modes that matter rather than one of them alone.

Reference: [`motion`](../../../../api/eng/structure/dynamics/motion.md), [`MotionResult`](../../../../api/eng/structure/dynamics/MotionResult.md), [`MotionOptions`](../../../../api/eng/structure/dynamics/MotionOptions.md), [`MotionState`](../../../../api/eng/structure/dynamics/MotionState.md), [`modes`](../../../../api/eng/structure/spectral/modes.md).

Where next: [Harmonic response](harmonic_response.md) computes the steady response to a sinusoidal load at many frequencies in one call, and checks it against the mode sum.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="buckling.html">
    <span class="label">Previous</span>
    <span class="title">Buckling</span>
  </a>
  <a class="next-link" href="harmonic_response.html">
    <span class="label">Next</span>
    <span class="title">Harmonic response</span>
  </a>
</div>
<!-- apiref:nav:end -->
