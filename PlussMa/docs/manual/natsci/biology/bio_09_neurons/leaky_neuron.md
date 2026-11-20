<!-- generated from leaky_neuron.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="firing_patterns.html">
    <span class="label">Next</span>
    <span class="title">Firing patterns</span>
  </a>
</div>
<!-- apiref:nav:end -->
