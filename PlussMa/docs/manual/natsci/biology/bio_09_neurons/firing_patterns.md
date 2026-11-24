# Firing patterns

[Back to Biology 9: Neurons](../bio_09_neurons.md)

Real neurons do more than fire at a steady rate: some burst, some adapt, some respond best to a rhythm. The Izhikevich model reproduces these patterns with two variables and four parameters. This page runs four parameter sets under the same input and compares how each fires.

## How it works

The state is the membrane voltage `v` and a recovery variable `u`. When `v` reaches `30` mV the neuron fires, `v` is reset to `c` and `u` is increased by `d`; `a` and `b` set the speed and the sensitivity of the recovery. `simulate_izhikevich(state0, current, params, options)` runs the model with every parameter possibly an array, so the four neurons below are four lanes of a single call, and returns the spike counts and the spike times of every lane, with `-1` marking unused slots.

## Four neurons, one input

The four lanes use the usual parameter sets for a regular-spiking, a chattering, a low-threshold-spiking and a fast-spiking neuron, all driven by a constant current of `10` for `500` ms. The program prints the spike counts and the first six spike times of each.

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
  // Izhikevich neurons: v starts at -65 mV, u at b v. Four classic parameter sets, one lane each.
  neuro::IzhikevichParams p;
  p.a = vec({0.02, 0.02, 0.02, 0.1});
  p.b = vec({0.2, 0.2, 0.25, 0.2});
  p.c = vec({-65.0, -50.0, -65.0, -65.0});
  p.d = vec({8.0, 2.0, 2.0, 2.0});
  const auto v0 = vec({-65.0, -65.0, -65.0, -65.0});
  const auto u0 = v0.mul(vec({0.2, 0.2, 0.25, 0.2}));
  const auto state0 = v0.unsqueeze(-1).unite(std::vector<pluss::Series>{u0.unsqueeze(-1)}, -1);
  neuro::SpikeOptions options;
  options.dt = 0.1;
  options.t_end = 500.0;
  options.max_spikes = 200;
  const auto run = neuro::simulate_izhikevich(state0, 10.0, p, options);
  out.writeln("regular spiking, chattering, low-threshold spiking, fast spiking: spikes in 500 ms = ", flat(run.count));
  const auto t = flat(run.times);
  const std::vector<std::string> names = {"regular spiking", "chattering", "low-threshold spiking", "fast spiking"};
  for (int lane = 0; lane < 4; ++lane) {
    std::vector<double> first;
    for (int k = 0; k < 6; ++k) first.push_back(t[lane * 200 + k]);
    out.writeln(names[lane], ": first spike times (ms) ", first);
  }
}
```

Output

```text
regular spiking, chattering, low-threshold spiking, fast spiking: spikes in 500 ms = [12.0000, 47.0000, 41.0000, 69.0000]
regular spiking: first spike times (ms) [3.1198, 26.2071, 71.0284, 115.8296, 160.6307, 205.4316]
chattering: first spike times (ms) [3.1198, 4.5057, 6.0190, 7.7078, 9.6326, 11.9397]
low-threshold spiking: first spike times (ms) [2.4602, 5.3212, 8.7757, 13.2016, 19.4326, 29.1932]
fast spiking: first spike times (ms) [3.1438, 7.4226, 13.2748, 20.2761, 27.5720, 34.9045]
```

The same input gives four different behaviours. The regular-spiking neuron settles to about one spike per `45` ms after an initial delay (`26.21`, `71.03`, `115.83`, ...). The chattering neuron fires in a quick burst (spikes at `3.12`, `4.51`, `6.02`, `7.71`, `9.63`, ...) with gaps widening inside the burst; the low-threshold and fast-spiking neurons begin with short intervals that lengthen, and by the end of `500` ms the counts are `12`, `47`, `41` and `69` spikes.

<!-- addex:start -->
## Input current and firing

The parameters and the input current of the Izhikevich model may be arrays, so a whole input-output curve is one simulation. The program drives a regular-spiking neuron with five currents for `500` ms and prints the spike counts and the time of the first spike for each, with `-1` meaning that no spike occurred.

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
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // a regular-spiking Izhikevich neuron under five different input currents in one call
  const auto current = vec({2.0, 4.0, 6.0, 10.0, 20.0});
  const auto v0 = vec({-65.0, -65.0, -65.0, -65.0, -65.0});
  const auto state0 = v0.unsqueeze(-1).unite(std::vector<pluss::Series>{v0.mul(0.2).unsqueeze(-1)}, -1);
  neuro::SpikeOptions options; options.dt = 0.1; options.t_end = 500.0; options.max_spikes = 200;
  const auto run = neuro::simulate_izhikevich(state0, current, {}, options);
  out.writeln("input current      ", flat(current));
  out.writeln("spikes in 500 ms   ", flat(run.count));
  out.writeln("first spike (ms)   ", std::vector<double>{flat(run.times)[0], flat(run.times)[200], flat(run.times)[400], flat(run.times)[600], flat(run.times)[800]});
}
```

Output

```text
input current      [2.00, 4.00, 6.00, 10.00, 20.00]
spikes in 500 ms   [0.00, 4.00, 7.00, 12.00, 24.00]
first spike (ms)   [-1.00, 12.23, 5.38, 3.12, 1.80]
```

A current of `2` produces no spikes, `4` produces `4`, and the counts grow with the current to `24` for a current of `20`; the first spike comes earlier as the current rises, `12.23` ms, `5.38` ms, `3.12` ms and `1.80` ms.

## Rest, oscillation and rest again

The FitzHugh-Nagumo model reduces a spiking neuron to two variables. As the input current rises the resting state loses stability and an oscillation appears, and then at a larger current it recovers. `fitzhugh_nagumo_hopf_currents` returns the two currents at which this happens. The program simulates four currents and prints the range of the voltage after the transient has passed.

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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // FitzHugh-Nagumo: below a first threshold current the neuron rests, between the two Hopf currents it oscillates, above the second it rests again
  const auto hopf = neuro::fitzhugh_nagumo_hopf_currents(0.7, 0.8, 0.08, vec({0.0}));
  out.writeln("Hopf currents = ", flat(hopf));
  for (double current : {0.0, 0.5, 1.0, 2.0}) {
    const auto rate = [&](const pluss::Series &s) { return neuro::fitzhugh_nagumo_rate(s, current); };
    auto s = vec({-1.0, 1.0});
    s = dynamics::simulate(rate, s, 0.0, 200.0, 8000);      // let the transient pass
    double lo = 1e9, hi = -1e9;
    for (int k = 0; k < 100; ++k) { s = dynamics::simulate(rate, s, 0.0, 1.0, 50); const double v = flat(s)[0]; lo = std::min(lo, v); hi = std::max(hi, v); }
    out.writeln("current ", current, ": voltage range over the next 100 time units = [", lo, ", ", hi, "]");
  }
}
```

Output

```text
Hopf currents = [0.3313, 1.4187]
current 0.0000: voltage range over the next 100 time units = [-1.1994, -1.1994]
current 0.5000: voltage range over the next 100 time units = [-1.9692, 1.8468]
current 1.0000: voltage range over the next 100 time units = [-1.9017, 1.9397]
current 2.0000: voltage range over the next 100 time units = [1.3341, 1.3341]
```

The two Hopf currents are `0.3313` and `1.4187`. At a current of `0` the voltage is steady at `-1.1994` (range of zero width); at `0.5` and `1.0`, between the two currents, it oscillates between about `-1.9` and `1.9`; at `2.0`, above the second current, it settles again at `1.3341`.

<!-- addex:end -->

## Decisions

- Use `simulate_izhikevich` when you want patterns (bursting, adaptation); use `simulate_lif` when only the rate matters.
- Give every lane its own parameters in one call rather than looping.
- Always use `count` to read the spike times: only the first `count` slots are real.

## Pitfalls

Unused spike slots hold `-1`, so averaging a whole row mixes real spike times with the filler. The program simulates a regular-spiking neuron for `100` ms with `20` slots and averages the times two ways.

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
  // unused spike slots hold -1: averaging the whole row mixes real spike times with the filler
  neuro::IzhikevichParams p;
  p.a = vec({0.02});
  p.b = vec({0.2});
  p.c = vec({-65.0});
  p.d = vec({8.0});
  const auto v0 = vec({-65.0});
  const auto state0 = v0.unsqueeze(-1).unite(std::vector<pluss::Series>{v0.mul(0.2).unsqueeze(-1)}, -1);
  neuro::SpikeOptions options;
  options.dt = 0.1;
  options.t_end = 100.0;
  options.max_spikes = 20;
  const auto run = neuro::simulate_izhikevich(state0, 10.0, p, options);
  const auto t = flat(run.times);
  const int count = static_cast<int>(flat(run.count)[0]);
  double all = 0.0, real = 0.0;
  for (int k = 0; k < 20; ++k) all += t[k];
  for (int k = 0; k < count; ++k) real += t[k];
  out.writeln("spikes ", count, ", slots ", 20, ", mean of all slots ", all / 20, ", mean of the real spikes ", real / count);
}
```

Output

```text
spikes 3, slots 20, mean of all slots 4.1678, mean of the real spikes 33.4518
```

The neuron fires `3` times. The mean over all `20` slots is `4.1678`, which is meaningless, because `17` of the slots are `-1`; the mean over the `3` real spikes is `33.4518` ms. Use the count to slice the row.

## Where next

[The leaky neuron](leaky_neuron.md) is the simpler model with a closed-form interval, and [Biology 16: Membranes](../bio_16_membranes.md) derives the currents behind these models.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="leaky_neuron.html">
    <span class="label">Previous</span>
    <span class="title">The leaky neuron</span>
  </a>
</div>
<!-- apiref:nav:end -->
