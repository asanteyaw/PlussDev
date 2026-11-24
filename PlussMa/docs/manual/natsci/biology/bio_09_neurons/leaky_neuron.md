# The leaky neuron

[Back to Biology 9: Neurons](../bio_09_neurons.md)

The simplest neuron model treats the membrane as a leaky capacitor that charges toward a drive and fires when its voltage reaches a threshold. This page finds the firing interval of such a neuron in closed form, then simulates the same neurons and counts their spikes.

## How it works

The voltage obeys `tau dV/dt = -(V - rest) + R I`: it relaxes to the rest voltage plus `R I` with time constant `tau`. When it reaches the threshold the neuron fires and the voltage is reset. If `rest + R I` is below the threshold the neuron never fires; above it, the interval between spikes is `tau ln((drive - reset)/(drive - threshold))`, and `lif_interspike_interval` returns that number, or infinity when the drive is too weak. `simulate_lif(v0, tau, rest, resistance, current, threshold, reset, options)` integrates the voltage with `options.dt` for `options.t_end` and returns, for every lane, the number of spikes `count` and the recorded spike `times`, located inside the step by interpolation. The currents form one array, so all neurons run together.

## Four currents

The neuron has `tau = 10` ms, rest `-65` mV, resistance `10` megohm, threshold `-50` mV and reset `-65` mV, so it needs `1.5` nA to fire. The program computes the exact intervals for currents of `1.0`, `1.6`, `2.0` and `3.0` nA, then simulates all four for `200` ms.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace neuro = biology::neuroscience;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a leaky neuron: membrane time constant 10 ms, rest -65 mV, resistance 10 Mohm, threshold -50 mV, reset -65 mV
  const double tau = 10.0, rest = -65.0, resistance = 10.0, threshold = -50.0, reset = -65.0;
  // injected currents in nA: below, near and above the threshold current of 1.5 nA
  const auto current = vec({1.0, 1.6, 2.0, 3.0});
  const auto like = current;
  const auto isi = neuro::lif_interspike_interval(tau, rest, resistance, current, threshold, reset, like);
  out.writeln("current (nA)                  ", flat(current));
  out.writeln("exact interspike interval (ms) ", flat(isi));

  // the same neurons simulated for 200 ms, all four at once
  neuro::SpikeOptions options;
  options.dt = 0.05;
  options.t_end = 200.0;
  const auto run = neuro::simulate_lif(vec({rest, rest, rest, rest}), tau, rest, resistance, current, threshold, reset, options);
  out.writeln("spikes in 200 ms               ", flat(run.count));
  out.writeln("first three spike times (ms), 2.0 nA neuron: ", std::vector<double>{flat(run.times.select(0, 2))[0], flat(run.times.select(0, 2))[1], flat(run.times.select(0, 2))[2]});
}
```

Output

```text
current (nA)                  [1.0000, 1.6000, 2.0000, 3.0000]
exact interspike interval (ms) [inf, 27.7259, 13.8629, 6.9315]
spikes in 200 ms               [0.0000, 7.0000, 14.0000, 28.0000]
first three spike times (ms), 2.0 nA neuron: [13.8630, 27.7259, 41.5889]
```

At `1.0` nA the drive is `-55` mV, below the threshold, and the interval is infinite, so the neuron stays silent. The other three fire with intervals of `27.7259`, `13.8629` and `6.9315` ms. The simulation agrees: `0`, `7`, `14` and `28` spikes in `200` ms, and the first spike times of the `2.0` nA neuron, `13.8630`, `27.7259` and `41.5889` ms, sit on multiples of the exact interval to the digits printed.

<!-- addex:start -->
## The current-frequency curve

The firing rate is the inverse of the interval, so the exact interval gives the whole curve of firing rate against input current. The program evaluates seven currents, printing the interval, the rate in spikes per second and the drive voltage {math}`E_L + RI` that the membrane relaxes toward.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace neuro = biology::neuroscience;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // the current-frequency curve of a leaky neuron: the firing rate is 1000 / (interspike interval in ms) spikes per second
  const auto current = vec({1.4, 1.5, 1.6, 2.0, 3.0, 5.0, 10.0});
  const auto isi = neuro::lif_interspike_interval(10.0, -65.0, 10.0, current, -50.0, -65.0, current);
  out.writeln("current (nA)       ", flat(current));
  out.writeln("interval (ms)      ", flat(isi));
  out.writeln("rate (spikes/s)    ", flat(isi.reciprocal().mul(1000.0)));
  // the voltage the neuron relaxes to for each current, drive = rest + R I, against the threshold of -50 mV
  out.writeln("drive voltage (mV) ", flat(current.mul(10.0).add(-65.0)));
}
```

Output

```text
current (nA)       [1.400, 1.500, 1.600, 2.000, 3.000, 5.000, 10.000]
interval (ms)      [inf, inf, 27.726, 13.863, 6.931, 3.567, 1.625]
rate (spikes/s)    [0.000, 0.000, 36.067, 72.135, 144.270, 280.367, 615.313]
drive voltage (mV) [-51.000, -50.000, -49.000, -45.000, -35.000, -15.000, 35.000]
```

Below `1.5` nA the drive voltage is below the `-50` mV threshold and the interval is infinite; at `1.6` nA the neuron fires every `27.726` ms (`36.067` spikes per second), and the rate grows with the current, `144.270` at `3` nA, `280.367` at `5` and `615.313` at `10`. After the steep onset the curve is close to linear: `56` to `62` spikes per second for each nanoampere between `5` and `10` nA.

## The time constant

A neuron with a smaller membrane time constant charges faster. The program takes three time constants in one batch, all driven with `2` nA, and compares the exact interval with the simulated spike count in `100` ms.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace neuro = biology::neuroscience;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // three membrane time constants in one batch, all driven with 2 nA: the exact interval and the simulated spike count in 100 ms
  const auto tau = vec({5.0, 10.0, 20.0});
  const auto isi = neuro::lif_interspike_interval(tau, -65.0, 10.0, 2.0, -50.0, -65.0, tau);
  out.writeln("time constant (ms)       ", flat(tau));
  out.writeln("exact interval (ms)      ", flat(isi));
  neuro::SpikeOptions options; options.dt = 0.05; options.t_end = 100.0;
  const auto run = neuro::simulate_lif(vec({-65.0, -65.0, -65.0}), tau, -65.0, 10.0, 2.0, -50.0, -65.0, options);
  out.writeln("spikes in 100 ms         ", flat(run.count));
  out.writeln("exact count (100 / interval) ", flat(isi.reciprocal().mul(100.0)));
}
```

Output

```text
time constant (ms)       [5.000, 10.000, 20.000]
exact interval (ms)      [6.931, 13.863, 27.726]
spikes in 100 ms         [14.000, 7.000, 3.000]
exact count (100 / interval) [14.427, 7.213, 3.607]
```

The intervals scale with the time constant, `6.931`, `13.863` and `27.726` ms for `5`, `10` and `20` ms, and the simulated counts are `14`, `7` and `3`, the integer parts of the exact `14.427`, `7.213` and `3.607`.

## Edge cases

A drive exactly at threshold never reaches it, and the interval is reported as infinity instead of an error; an invalid simulation setting is an error. The program prints the intervals for currents just at and above `1.5` nA, and passes a zero time step to the simulator.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace neuro = biology::neuroscience;
int main() {
  OutputEngine out;
  // a simulation needs a positive step and duration
  try {
    neuro::SpikeOptions options; options.dt = 0.0;
    (void)neuro::simulate_lif(vec({-65.0}), 10.0, -65.0, 10.0, 2.0, -50.0, -65.0, options);
  } catch (const pluss::Error &e) {
    out.writeln("zero time step: ", e.message());
  }
  // a subthreshold drive never fires, and the interval is reported as infinity rather than as an error
  const auto isi = neuro::lif_interspike_interval(10.0, -65.0, 10.0, vec({1.0, 1.5, 1.51}), -50.0, -65.0, vec({1.0, 1.5, 1.51}));
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  out.writeln("interval at 1.0, 1.5, 1.51 nA = ", flat(isi));
}
```

Output

```text
zero time step:  natsci: spiking: dt and t_end must be positive
interval at 1.0, 1.5, 1.51 nA = [inf, inf, 50.173]
```

At `1.0` and at exactly `1.5` nA the interval is `inf`; at `1.51` nA it is `50.173` ms, which shows how steeply the interval falls just above the threshold. A time step of zero raises a `pluss::Error` that states the requirement.

<!-- addex:end -->

## Decisions

- Use `lif_interspike_interval` for the rate of a steadily driven neuron; simulate only when the input varies in time.
- Choose `dt` well below `tau`: spikes are counted per step.

## Pitfalls

The simulation advances in steps of `dt`, and a step comparable to the membrane time constant misses spikes. The program counts the spikes of the page's `2` nA neuron in `200` ms with four step sizes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace neuro = biology::neuroscience;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the step must be small against the membrane time constant: a coarse step misses spikes
  const auto current = vec({2.0});
  for (double dt : {0.05, 2.0, 10.0, 20.0}) {
    neuro::SpikeOptions options;
    options.dt = dt;
    options.t_end = 200.0;
    const auto run = neuro::simulate_lif(vec({-65.0}), 10.0, -65.0, 10.0, current, -50.0, -65.0, options);
    out.writeln("dt = ", dt, ": spikes in 200 ms = ", flat(run.count));
  }
  out.writeln("exact count = 200 / 13.8629 = ", 200.0 / 13.8629);
}
```

Output

```text
dt = 0.0500: spikes in 200 ms = [14.0000]
dt = 2.0000: spikes in 200 ms = [14.0000]
dt = 10.0000: spikes in 200 ms = [13.0000]
dt = 20.0000: spikes in 200 ms = [9.0000]
exact count = 200 / 13.8629 = 14.4270
```

With `dt = 0.05` and `2` ms the count is `14`, in agreement with the exact `200/13.8629 = 14.4270` rounded down; with `10` ms (the time constant itself) it is `13`, and with `20` ms only `9`. If a spike count does not match the exact interval, shrink `dt` first.

## Where next

[Firing patterns](firing_patterns.md) lets the neuron adapt and burst, and [Biology 16: Membranes](../bio_16_membranes.md) gives the conductances the leak stands for.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="firing_patterns.html">
    <span class="label">Next</span>
    <span class="title">Firing patterns</span>
  </a>
</div>
<!-- apiref:nav:end -->
