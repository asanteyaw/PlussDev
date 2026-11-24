# Impulse conduction

[Back to Biology 39: Axons](../bio_39_axons.md)

A nerve fibre carries a signal over distances that are long compared with the fibre itself, and you want to know how fast the impulse travels, how that speed depends on the size of the fibre, and what a fibre without active channels does to a signal instead. This page propagates a Hodgkin-Huxley impulse along an axon, times it at two points to measure its speed, repeats the measurement for fibres of different radii, and compares with a passive cable whose response decays with distance.

## How it works

The code is in `pluss::ma::natsci::biology::biophysics::membrane` (`membrane::` below). The membrane voltage of a fibre of radius {math}`a` and axoplasm resistivity {math}`R_a` obeys a reaction-diffusion equation whose reaction is the Hodgkin-Huxley ionic current,

```{math}
C_m\frac{\partial V}{\partial t} = \frac{a}{2R_a}\frac{\partial^2 V}{\partial x^2} - I_{\mathrm{ion}}(V, m, h, n)
```

with the three gating variables {math}`m`, {math}`h` and {math}`n` evolving at each point. `membrane::hodgkin_huxley_axon_rate(state, radius_cm, axoplasm_ohm_cm, grid, parameters)` is the right-hand side for a state of shape {math}`[4, N]` (the voltage and the three gates along {math}`N` nodes), on a one-dimensional periodic grid made with `spatial::make_periodic_grid`. The grid stores its spacing as a number of length units; this model's own units are centimetres, milliseconds, millivolts and microfarads, so the spacing is given in centimetres. `membrane::hodgkin_huxley_rest(like)` gives the resting values of the four variables, and `membrane::HodgkinHuxley` holds the conductances, the reversal potentials and the temperature (`celsius`, default `6.3`). `membrane::simulate_axon(state, radius, resistivity, grid, t0, t1, steps)` integrates it and refuses a step beyond `membrane::axon_max_stable_dt(radius, resistivity, grid)`, the largest stable step. Without active channels the fibre is a passive cable, {math}`\tau\,\partial_t V = \lambda^2 \partial_x^2 V - V + \text{drive}`, which is `membrane::cable_rate(v, lambda, tau, grid, drive)`; its steady response to a point input falls off as {math}`e^{-|x|/\lambda}`. Integrate the passive cable with `dynamics::simulate`; the explicit rule needs a step below about {math}`\Delta x^2/D`, where {math}`D = a/(2R_aC_m)` is the diffusivity of the voltage, and `simulate_axon` checks that for the axon.

## An impulse travels

The program lays the axon on a ring of `600` nodes `0.02` cm apart, depolarises the first twenty nodes (`0.4` cm) to `40` mV and reports where the largest voltage on the first half of the ring is at `1`, `2`, `3` and `4` ms, with the voltage there and the speed between rows.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace dynamics = common::dynamics;
namespace membrane = natsci::biology::biophysics::membrane;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// a ring of n nodes, dx centimetres apart (the grid stores the spacing as a plain number, so the length unit is whatever the model uses: here cm)
static spatial::PeriodicGrid ring(std::int64_t n, double dx) { const auto h = common::ScalarQuantity<common::Length>::from_si(dx); return spatial::make_periodic_grid({n}, {h}); }
// the resting state of every node, with the first few nodes depolarised to start a pulse
static pluss::Series start_state(std::int64_t n, std::int64_t kick) {
  const auto rest = membrane::hodgkin_huxley_rest(vec({0.0}));                    // (V, m, h, n) at rest
  auto s = rest.reshape({4, 1}).expand({4, n}, true).contiguous();
  auto v = flat(s.select(0, 0)); for (std::int64_t i = 0; i < kick; ++i) v[i] = 40.0;
  return pluss::series::make(v).to(pluss::ScalarType::Float64).unsqueeze(0).unite(std::vector<pluss::Series>{s.slice(0, 1, 4, 1)}, 0);
}
// position (in cm) of the largest voltage on the right half of the ring
static double front(const pluss::Series &s, std::int64_t n, double dx) {
  const auto v = flat(s.select(0, 0)); std::int64_t best = n / 2; double top = -1e9;
  for (std::int64_t i = 0; i < n / 2; ++i) if (v[i] > top) { top = v[i]; best = i; }
  return best * dx;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // a squid-like axon: radius 0.0238 cm, axoplasm resistivity 35.4 ohm cm, on a ring of 600 nodes 0.02 cm apart (12 cm long)
  const std::int64_t n = 600;
  const double dx = 0.02, radius = 0.0238, ra = 35.4;
  const auto grid = ring(n, dx);
  auto s = start_state(n, 20);                      // the first twenty nodes (0.4 cm) are depolarised to 40 mV: two pulses set off, one in each direction
  double t = 0.0, last = 0.0;
  for (double target : {1.0, 2.0, 3.0, 4.0}) {
    s = membrane::simulate_axon(s, radius, ra, grid, t, target, static_cast<std::int64_t>((target - t) / 0.0005));
    t = target;
    const double x = front(s, n, dx);
    out.writeln("time ", t, " ms: pulse peak at x = ", x, " cm, voltage there ", flat(s.select(0, 0))[static_cast<std::int64_t>(x / dx + 0.5)], " mV");
    if (target > 1.0) out.writeln("   speed since the last row = ", (x - last) / 1.0 * 10.0, " m/s");
    last = x;
  }
}
```

Output

```text
time 1.000 ms: pulse peak at x = 0.180 cm, voltage there 45.739 mV
time 2.000 ms: pulse peak at x = 1.180 cm, voltage there 102.678 mV
   speed since the last row = 10.000 m/s
time 3.000 ms: pulse peak at x = 2.420 cm, voltage there 102.938 mV
   speed since the last row = 12.400 m/s
time 4.000 ms: pulse peak at x = 3.660 cm, voltage there 102.966 mV
   speed since the last row = 12.400 m/s
```

At `1` ms the peak is still at the stimulated end (`0.180` cm, `45.739` mV) while the impulse forms. By `2` ms it has reached its full height of about `103` mV and then travels with that height and a steady speed: `2.420` cm at `3` ms and `3.660` cm at `4` ms, `12.400` m/s in each of the last two intervals. A pulse that keeps its amplitude while it moves is what distinguishes an impulse from the passive spread of a signal.

## Fibre radius and speed

A larger fibre has more current-carrying cross-section for the same membrane area. The program times the impulse at `2` cm and at `4` cm from the stimulated end, for three radii differing by factors of two, and prints the speed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <functional>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace dynamics = common::dynamics;
namespace membrane = natsci::biology::biophysics::membrane;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// a ring of n nodes, dx centimetres apart (the grid stores the spacing as a plain number, so the length unit is whatever the model uses: here cm)
static spatial::PeriodicGrid ring(std::int64_t n, double dx) { const auto h = common::ScalarQuantity<common::Length>::from_si(dx); return spatial::make_periodic_grid({n}, {h}); }
// the resting state of every node, with the first few nodes depolarised to start a pulse
static pluss::Series start_state(std::int64_t n, std::int64_t kick) {
  const auto rest = membrane::hodgkin_huxley_rest(vec({0.0}));                    // (V, m, h, n) at rest
  auto s = rest.reshape({4, 1}).expand({4, n}, true).contiguous();
  auto v = flat(s.select(0, 0)); for (std::int64_t i = 0; i < kick; ++i) v[i] = 40.0;
  return pluss::series::make(v).to(pluss::ScalarType::Float64).unsqueeze(0).unite(std::vector<pluss::Series>{s.slice(0, 1, 4, 1)}, 0);
}
// position (in cm) of the largest voltage on the right half of the ring
static double front(const pluss::Series &s, std::int64_t n, double dx) {
  const auto v = flat(s.select(0, 0)); std::int64_t best = n / 2; double top = -1e9;
  for (std::int64_t i = 0; i < n / 2; ++i) if (v[i] > top) { top = v[i]; best = i; }
  return best * dx;
}

// advance the axon 0.05 ms at a time and return the times at which the voltage first passes 50 mV at two nodes
static std::pair<double, double> crossing_times(double radius, double ra, const spatial::PeriodicGrid &grid, pluss::Series s, std::int64_t first, std::int64_t second, double dt) {
  double t = 0.0, t_first = -1.0, t_second = -1.0;
  while (t < 12.0 && t_second < 0.0) {
    s = membrane::simulate_axon(s, radius, ra, grid, t, t + 0.05, static_cast<std::int64_t>(0.05 / dt) + 1);
    t += 0.05;
    const auto v = flat(s.select(0, 0));
    if (t_first < 0.0 && v[first] > 50.0) t_first = t;
    if (t_second < 0.0 && v[second] > 50.0) t_second = t;
  }
  return {t_first, t_second};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // the same axon at three radii: the pulse is timed at 2 cm and at 4 cm from the stimulated end, and the speed is the distance over the time between them
  const std::int64_t n = 800;
  const double dx = 0.02, ra = 35.4;
  const auto grid = ring(n, dx);
  for (double radius : {0.0119, 0.0238, 0.0476}) {
    const double dt = 0.25 * dx * dx / (1000.0 * radius / (2.0 * ra));
    const auto [t2, t4] = crossing_times(radius, ra, grid, start_state(n, 40), 100, 200, dt);       // nodes 100 and 200 are 2 cm and 4 cm along
    const double speed = 2.0 / (t4 - t2) * 10.0;
    out.writeln("radius ", radius, " cm: crossing times ", t2, " and ", t4, " ms, speed ", speed, " m/s, speed / sqrt(radius) = ", speed / std::sqrt(radius));
  }
}
```

Output

```text
radius 0.012 cm: crossing times 1.850 and 4.150 ms, speed 8.696 m/s, speed / sqrt(radius) = 79.713
radius 0.024 cm: crossing times 1.500 and 3.100 ms, speed 12.500 m/s, speed / sqrt(radius) = 81.025
radius 0.048 cm: crossing times 1.300 and 2.450 ms, speed 17.391 m/s, speed / sqrt(radius) = 79.713
```

The speed rises with the radius, from `8.696` m/s at `0.0119` cm to `12.500` m/s at `0.0238` cm and `17.391` m/s at `0.0476` cm, and speed divided by {math}`\sqrt{a}` is nearly constant (`79.7`, `81.0`, `79.7`): the speed grows as the square root of the radius, because the diffusivity {math}`a/(2R_aC_m)` does. A fibre four times as wide conducts twice as fast, which is why large invertebrate axons are thick.

## A passive cable

Without active channels the signal cannot be regenerated. The program injects a steady current at the middle of a passive cable with {math}`\lambda = 0.1` cm and {math}`\tau = 2` ms and compares the steady voltage at four distances with {math}`e^{-x/\lambda}`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace dynamics = common::dynamics;
namespace membrane = natsci::biology::biophysics::membrane;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// a ring of n nodes, dx centimetres apart (the grid stores the spacing as a plain number, so the length unit is whatever the model uses: here cm)
static spatial::PeriodicGrid ring(std::int64_t n, double dx) { const auto h = common::ScalarQuantity<common::Length>::from_si(dx); return spatial::make_periodic_grid({n}, {h}); }
// the resting state of every node, with the first few nodes depolarised to start a pulse
static pluss::Series start_state(std::int64_t n, std::int64_t kick) {
  const auto rest = membrane::hodgkin_huxley_rest(vec({0.0}));                    // (V, m, h, n) at rest
  auto s = rest.reshape({4, 1}).expand({4, n}, true).contiguous();
  auto v = flat(s.select(0, 0)); for (std::int64_t i = 0; i < kick; ++i) v[i] = 40.0;
  return pluss::series::make(v).to(pluss::ScalarType::Float64).unsqueeze(0).unite(std::vector<pluss::Series>{s.slice(0, 1, 4, 1)}, 0);
}
// position (in cm) of the largest voltage on the right half of the ring
static double front(const pluss::Series &s, std::int64_t n, double dx) {
  const auto v = flat(s.select(0, 0)); std::int64_t best = n / 2; double top = -1e9;
  for (std::int64_t i = 0; i < n / 2; ++i) if (v[i] > top) { top = v[i]; best = i; }
  return best * dx;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a passive cable: length constant lambda = 0.1 cm, time constant tau = 2 ms, constant current injected at the middle node of a ring of 200 nodes
  const std::int64_t n = 200;
  const double dx = 0.01, lambda = 0.1, tau = 2.0;
  const auto grid = ring(n, dx);
  std::vector<double> drive(n, 0.0); drive[n / 2] = 1.0 / dx;   // unit current spread over one node
  const auto input = vec(drive);
  const auto rate = [&](const pluss::Series &v) { return membrane::cable_rate(v, lambda, tau, grid, input); };
  const auto v = dynamics::simulate(rate, vec(std::vector<double>(n, 0.0)), 0.0, 40.0, 8000);
  const auto vv = flat(v);
  const double v0 = vv[n / 2];
  out.writeln("voltage at the injection site = ", v0);
  for (double d : {0.05, 0.1, 0.2, 0.3}) {
    const auto k = static_cast<std::int64_t>(std::llround(d / dx));
    out.writeln("distance ", d, " cm (", d / lambda, " lambda): relative voltage ", vv[n / 2 + k] / v0, ", exp(-x/lambda) = ", std::exp(-d / lambda));
  }
}
```

Output

```text
voltage at the injection site = 4.9938
distance 0.0500 cm (0.5000 lambda): relative voltage 0.6067, exp(-x/lambda) = 0.6065
distance 0.1000 cm (1.0000 lambda): relative voltage 0.3680, exp(-x/lambda) = 0.3679
distance 0.2000 cm (2.0000 lambda): relative voltage 0.1354, exp(-x/lambda) = 0.1353
distance 0.3000 cm (3.0000 lambda): relative voltage 0.0498, exp(-x/lambda) = 0.0498
```

The voltage at the injection site is `4.9938`. At `0.5` length constants it is `0.6067` of that against {math}`e^{-0.5} = 0.6065`, at one it is `0.3680` against `0.3679`, at two `0.1354` against `0.1353`, and at three `0.0498` against `0.0498`. The signal decays by a factor {math}`e` for every length constant, so a passive fibre of a few length constants cannot carry a signal over a long distance, and this is why the active fibre above, which regenerates the impulse at every node, is needed.

## Decisions

- Use `hodgkin_huxley_axon_rate` to study conduction and `cable_rate` for the passive spread of small, sub-threshold signals, where the two agree.
- Time the impulse at two fixed points rather than reading the position at two times; the speed is then free of the start-up transient, as the first example shows.
- Choose the step from the diffusive limit {math}`\Delta x^2/D` before the run, and halve the node spacing to confirm that the speed does not change.

## Pitfalls

The explicit step must respect the diffusive limit, and the four variables sit on the second-to-last axis. The program asks for the largest stable step, runs the axon for `1` ms with a step inside it and with a step ten times larger, and passes a state laid out as {math}`[N, 4]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <functional>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace dynamics = common::dynamics;
namespace membrane = natsci::biology::biophysics::membrane;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// a ring of n nodes, dx centimetres apart (the grid stores the spacing as a plain number, so the length unit is whatever the model uses: here cm)
static spatial::PeriodicGrid ring(std::int64_t n, double dx) { const auto h = common::ScalarQuantity<common::Length>::from_si(dx); return spatial::make_periodic_grid({n}, {h}); }
// the resting state of every node, with the first few nodes depolarised to start a pulse
static pluss::Series start_state(std::int64_t n, std::int64_t kick) {
  const auto rest = membrane::hodgkin_huxley_rest(vec({0.0}));                    // (V, m, h, n) at rest
  auto s = rest.reshape({4, 1}).expand({4, n}, true).contiguous();
  auto v = flat(s.select(0, 0)); for (std::int64_t i = 0; i < kick; ++i) v[i] = 40.0;
  return pluss::series::make(v).to(pluss::ScalarType::Float64).unsqueeze(0).unite(std::vector<pluss::Series>{s.slice(0, 1, 4, 1)}, 0);
}
// position (in cm) of the largest voltage on the right half of the ring
static double front(const pluss::Series &s, std::int64_t n, double dx) {
  const auto v = flat(s.select(0, 0)); std::int64_t best = n / 2; double top = -1e9;
  for (std::int64_t i = 0; i < n / 2; ++i) if (v[i] > top) { top = v[i]; best = i; }
  return best * dx;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const std::int64_t n = 400;
  const double dx = 0.02, radius = 0.0238, ra = 35.4;
  const auto grid = ring(n, dx);
  out.writeln("largest stable step = ", membrane::axon_max_stable_dt(radius, ra, grid), " ms");
  const auto s = start_state(n, 20);
  const auto fine = membrane::simulate_axon(s, radius, ra, grid, 0.0, 1.0, 2000);        // step 0.0005 ms
  out.writeln("step 0.0005 ms: voltage at node 50 after 1 ms = ", flat(fine.select(0, 0))[50]);
  try {
    membrane::simulate_axon(s, radius, ra, grid, 0.0, 1.0, 200);                          // step 0.005 ms
  } catch (const pluss::Error &e) { out.writeln("step 0.005 ms: error: ", e.message()); }
  // the four variables sit on the second-to-last axis: [4, nodes], not [nodes, 4]
  try {
    membrane::hodgkin_huxley_axon_rate(s.transpose(0, 1), radius, ra, grid);
  } catch (const pluss::Error &e) { out.writeln("layout [nodes, 4]: error: ", e.message()); }
}
```

Output

```text
largest stable step = 0.0008 ms
step 0.0005 ms: voltage at node 50 after 1 ms = 9.0544
step 0.005 ms: error: natsci: simulate_axon: the step is too large for the diffusion along the fibre (see axon_max_stable_dt); use more steps
layout [nodes, 4]: error: natsci: hodgkin_huxley_axon: state must be [..., 4, N]
```

The largest stable step for this grid is `0.0008` ms (`axon_max_stable_dt`), and a step of `0.0005` ms runs and gives `9.0544` mV at node `50` after `1` ms. A step of `0.005` ms is refused with an error that names the limit, instead of silently overflowing into `nan`, and the layout {math}`[N, 4]` is refused with a message that names the shape the function needs. Take the step from `axon_max_stable_dt`, then halve it once to confirm that the result does not change.

## Where next

[Biology 9: Neurons](../bio_09_neurons.md) reduces the neuron to a point with a few variables, [Biology 16: Membranes](../bio_16_membranes.md) builds the Nernst and Goldman voltages that set the reversal potentials used here, and [Biology 40: Simulating Reaction-Diffusion](../bio_40_reaction_diffusion.md) treats the general reaction-diffusion grid.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
