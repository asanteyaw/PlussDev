# Ion voltages

[Back to Biology 16: Membranes](../bio_16_membranes.md)

A cell membrane separates solutions of different composition, and the ions that can cross it set the voltage across it. This page computes the voltage at which each ion is in balance, the resting voltage of a membrane that leaks several ions, and the action potential of the squid-axon membrane when it is stimulated.


## How it works

A membrane voltage is the balance of ion movements. `membrane::nernst(out, in, z, rt_over_f)` gives the voltage at which one ion is in balance, `membrane::ghk_voltage(perm, out, in, sign, rt_over_f)` the steady voltage of a membrane that leaks several ions weighted by their permeabilities, and `membrane::hodgkin_huxley_rate(state, current)` the time derivative of the squid-axon model, whose four-number state is the voltage from rest and three gates. The thermal voltage `rt_over_f` is in millivolts throughout, about `26.7` at body temperature. Everything takes arrays, so a set of ions, permeabilities or stimulus strengths is one call.

## The balance voltage of an ion

An ion at different concentrations on the two sides of a membrane is in balance when the electric force on it cancels the diffusion force. The voltage is the Nernst potential

```{math}
E = \frac{RT}{zF}\ln\frac{[\mathrm{out}]}{[\mathrm{in}]}
```

`membrane::nernst(outside, inside, valence, rt_over_f)` evaluates it, with `rt_over_f` the thermal voltage, {math}`26.7` mV at body temperature, and the concentrations as arrays. The program uses typical concentrations of a mammalian cell in mM for sodium, potassium, chloride and calcium.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace membrane = biology::biophysics::membrane;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // the voltage at which an ion is in balance (Nernst): concentrations in mM, RT/F = 26.7 mV at body temperature
  const auto outside = vec({145.0, 4.0, 110.0, 2.0});     // Na+, K+, Cl-, Ca2+
  const auto inside = vec({12.0, 140.0, 10.0, 0.0001});
  const auto valence = std::vector<double>{1.0, 1.0, -1.0, 2.0};
  std::vector<double> e;
  for (int i = 0; i < 4; ++i) e.push_back(flat(membrane::nernst(outside.select(0, i), inside.select(0, i), valence[i], 26.7))[0]);
  out.writeln("equilibrium voltages (mV) for Na, K, Cl, Ca = ", e);
}
```

Output

```text
equilibrium voltages (mV) for Na, K, Cl, Ca = [66.53, -94.93, -64.02, 132.21]
```

Potassium is in balance at `-94.93` mV, sodium at `66.53` mV, chloride at `-64.02` mV and calcium at `132.21` mV, the largest because calcium is concentrated outside by a factor of twenty thousand.

## The resting voltage

A membrane that is permeable to several ions rests at a voltage between their balance voltages, weighted by permeability, the Goldman-Hodgkin-Katz voltage. `membrane::ghk_voltage(permeability, outside, inside, sign, rt_over_f)` takes arrays over the ions, with `sign` {math}`+1` for cations and {math}`-1` for anions. The program leaks potassium, sodium and chloride, and raises the sodium permeability relative to potassium.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace membrane = biology::biophysics::membrane;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // the resting voltage of a cell that leaks several ions, weighted by their permeability (Goldman-Hodgkin-Katz): K+, Na+, Cl-
  const auto outside = vec({4.0, 145.0, 110.0}), inside = vec({140.0, 12.0, 10.0});
  const auto sign = vec({1.0, 1.0, -1.0});
  for (double p_na : {0.01, 0.05, 0.5, 5.0}) {
    const auto perm = vec({1.0, p_na, 0.45});
    out.writeln("Na permeability ", p_na, " relative to K: resting voltage = ", flat(membrane::ghk_voltage(perm, outside, inside, sign, 26.7))[0], " mV");
  }
}
```

Output

```text
Na permeability 0.01 relative to K: resting voltage = -78.70 mV
Na permeability 0.05 relative to K: resting voltage = -66.50 mV
Na permeability 0.50 relative to K: resting voltage = -23.53 mV
Na permeability 5.00 relative to K: resting voltage = 28.79 mV
```

A membrane that leaks mostly potassium rests near its voltage, `-78.70` mV when sodium is a hundredth as permeable. As the sodium permeability rises the resting voltage climbs to `-66.50`, `-23.53` and finally `28.79` mV when sodium is five times as permeable as potassium.

## An action potential

The Hodgkin-Huxley model describes the sodium and potassium channels of the squid axon by three gates {math}`m`, {math}`h` and {math}`n` that open and close with the voltage. `membrane::hodgkin_huxley_rate(s, i_ext, params)` is the right-hand side of the state `(V, m, h, n)`, with the voltage measured from rest in mV, the time in ms and the injected current in {math}`\mu`A/cm{math}`^2`; `membrane::hodgkin_huxley_rest(like)` is the resting state. The program gives the membrane a one-millisecond pulse of three current strengths and follows it for {math}`20` ms.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace membrane = biology::biophysics::membrane;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // an action potential: the Hodgkin-Huxley membrane (voltage measured from rest, in mV) given a short pulse of current
  const auto rate_with = [](double current) { return [current](const pluss::Series &s) { return membrane::hodgkin_huxley_rate(s, current); }; };
  for (double current : {2.0, 6.0, 10.0}) {
    auto s = membrane::hodgkin_huxley_rest(vec({0.0}));
    s = s.reshape({4});
    const auto pulse = rate_with(current), rest = rate_with(0.0);
    double peak = -1e9, t_peak = 0.0;
    for (int k = 0; k < 200; ++k) {
      s = dynamics::simulate(k < 10 ? pulse : rest, s, 0.0, 0.1, 10);     // current on for the first millisecond
      const double v = flat(s)[0];
      if (v > peak) { peak = v; t_peak = 0.1 * (k + 1); }
    }
    out.writeln("pulse of ", current, " uA/cm^2 for 1 ms: peak voltage ", peak, " mV at ", t_peak, " ms, voltage after 20 ms ", flat(s)[0], " mV");
  }
}
```

Output

```text
pulse of 2.00 uA/cm^2 for 1 ms: peak voltage 1.64 mV at 1.00 ms, voltage after 20 ms 0.02 mV
pulse of 6.00 uA/cm^2 for 1 ms: peak voltage 5.11 mV at 1.00 ms, voltage after 20 ms 0.10 mV
pulse of 10.00 uA/cm^2 for 1 ms: peak voltage 104.02 mV at 2.50 ms, voltage after 20 ms 0.31 mV
```

The membrane responds to a short pulse in an all-or-nothing way. Pulses of `2` and `6` {math}`\mu`A/cm{math}`^2` leave only a small bump (`1.64` and `5.11` mV, at the end of the pulse), while `10` triggers a full action potential with a peak of `104.02` mV at `2.50` ms; after `20` ms the voltage is back within `0.31` mV of rest.

## Decisions

- Use `nernst` to ask where one ion would pull the voltage, and `ghk_voltage` to find where the combination of leaks leaves it.
- Use `hodgkin_huxley_rate` with `dynamics::simulate` for anything that happens in time; start from `hodgkin_huxley_rest` so the gates are consistent with the voltage.
- Scan a stimulus strength or a permeability by making it a lane, not a loop.

## Pitfalls

Concentrations must be positive, the valence carries the sign, the voltages are in millivolts and the Hodgkin-Huxley state has four numbers. The program breaks each.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace systems = biology::systems;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
namespace membrane = biology::biophysics::membrane;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // a concentration of zero makes the Nernst voltage infinite
  try { out.writeln("zero inside: ", flat(membrane::nernst(vec({4.0}), vec({0.0}), 1.0, 26.7))); }
  catch (const pluss::Error &e) { out.writeln("zero inside: ", e.message()); }
  // the valence carries the sign: chloride is -1, and the same call with +1 gives the opposite voltage
  out.writeln("chloride with valence -1 and +1: ", std::vector<double>{flat(membrane::nernst(vec({110.0}), vec({10.0}), -1.0, 26.7))[0], flat(membrane::nernst(vec({110.0}), vec({10.0}), 1.0, 26.7))[0]});
  // the thermal voltage is in millivolts here: 0.0267 (volts) gives a voltage a thousand times too small
  out.writeln("potassium with RT/F = 26.7 and 0.0267: ", std::vector<double>{flat(membrane::nernst(vec({4.0}), vec({140.0}), 1.0, 26.7))[0], flat(membrane::nernst(vec({4.0}), vec({140.0}), 1.0, 0.0267))[0]});
  // the Hodgkin-Huxley state is (V, m, h, n): four numbers
  try { (void)membrane::hodgkin_huxley_rate(vec({0.0, 0.05}), 0.0); out.writeln("two numbers accepted"); }
  catch (const pluss::Error &e) { out.writeln("two numbers: ", e.message()); }
}
```

Output

```text
zero inside: natsci: nernst: inside must be strictly positive
chloride with valence -1 and +1: [-64.02, 64.02]
potassium with RT/F = 26.7 and 0.0267: [-94.93, -0.09]
two numbers: natsci: hodgkin_huxley: state must have 4 species on its last axis
```

A zero concentration is rejected because the logarithm of zero is not a voltage. A valence of `+1` for chloride gives `64.02` mV instead of `-64.02`: the valence, not the concentrations, decides the sign. A thermal voltage of `0.0267` (volts) with concentrations in the same call gives `-0.09` mV instead of `-94.93`, a thousand times too small. A state of two numbers is rejected: the model needs `(V, m, h, n)`.

## Where next

[Biology 9: Neurons](../bio_09_neurons.md) builds simpler neuron models from the same ideas, and [Biology 19: Diffusion](../bio_19_diffusion.md) treats how ions spread in space.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
