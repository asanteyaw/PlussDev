<!-- generated from describing_a_network.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Describing a network

[Back to Power 1: Networks and Power Flow](../power_01_networks_and_power_flow.md)

You have a transmission network: buses, the lines and transformers between them, generators and loads, and you want to study it. `power::make` takes a description of the network, a `Net`, checks that it is a network a power flow can solve, and compiles it into a `Grid` that every analysis function takes. This page explains how a network is written down: the per-unit system, the three kinds of bus, how generators and loads enter, the line parameters including transformers and charging, and the sweeps of scenarios that are written as batches.

## The mental model

Power-system quantities are written in **per unit**: each one is divided by a base, so that a 100 MW load on a 100 MVA base is `1.0`, and voltages sit near `1.0`. A `Base` holds two numbers, the three-phase apparent-power base in volt-amperes and the line-to-line voltage base in volts. From them the impedance base `zbase = V^2 / S`, the admittance base `ybase = 1 / zbase` and the current base `ibase = S / (sqrt(3) V)` follow, and `pu` and `si` convert a value to and from per unit.

A network is a set of **buses**, each with an injection of power, joined by **lines**. The unknowns are the voltage magnitude and angle at every bus, and the network equations say that the power injected at each bus equals the power flowing out through the lines. Three kinds of bus fix three different pairs of quantities:

| `BusKind` | Fixes | Solved for | Typical use |
|---|---|---|---|
| `Slack` | voltage magnitude and angle | active and reactive injection | the reference bus, absorbing the losses; exactly one |
| `PV` | active injection and voltage magnitude | reactive injection and angle | a generator bus with voltage control |
| `PQ` | active and reactive injection | voltage magnitude and angle | a load bus |

Injections are signed: a generator injects positive power and a load is negative. The description groups its data as `Buses` (a kind, the fixed injections `p` and `q`, the voltage set points `vm` and `va`, optional shunt conductance `g` and susceptance `b`), `Gens` (the buses where generators sit, and their `p` and `q`), and `Lines` (`from`, `to`, series resistance `r` and reactance `x`, total charging susceptance `charge`, off-nominal transformer `tap` and phase `shift`, and an `active` status). The net injection of a bus is its `p` plus the `p` of every active generator on it. All values are `Series` whose last axis lists the buses or lines; leading axes are batches of scenarios.

## The per-unit system

The first program converts between physical units and per unit on a `100 MVA`, `230 kV` base: it prints the three derived bases, converts a `50 MW` load to per unit and back, and converts a `52.9 ohm` line reactance, which should be `0.1` per unit on this base.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Base base{100e6, 230e3};   // 100 MVA, 230 kV line to line
  out.writeln("impedance base (ohm): ", power::zbase(base));
  out.writeln("admittance base (S):  ", power::ybase(base));
  out.writeln("current base (A):     ", power::ibase(base));
  out.writeln("50 MW in per unit: ", flat(power::pu(vec({50e6}), base.power)), "   0.5 per unit in watts: ", flat(power::si(vec({0.5}), base.power)));
  out.writeln("52.9 ohm in per unit: ", flat(power::pu(vec({52.9}), power::zbase(base))));
}
```

Output

```text
impedance base (ohm): 529.00000
admittance base (S):  0.00189
current base (A):     251.02186
50 MW in per unit: [0.50000]   0.5 per unit in watts: [50000000.00000]
52.9 ohm in per unit: [0.10000]
```

The impedance base is `230000^2 / 100e6 = 529 ohm`, so `52.9 ohm` is `0.1` per unit. Choose the bases once for a study and convert every input and output through them.

## Compiling a network

The simplest network is two buses and a line: a slack bus holding `1.0` per unit and a load bus drawing `0.5 + 0.2j` per unit through a line of `0.01 + 0.1j` per unit. `make` returns a `Grid`, which reports the counts and holds the **bus admittance matrix** `Y` that relates the bus current injections to the voltages. For a single line `Y` is `[[y, -y], [-y, y]]` with `y = 1 / (r + jx)`, which here is `0.990099 - 9.90099j`. The program builds the network, prints the counts and the admittance matrix, and compares the first entry with `1 / (r + jx)` computed directly.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.5});       // the load bus draws 0.5 per unit of active power
  net.buses.q = vec({0.0, -0.2});       // and 0.2 of reactive power
  net.buses.vm = vec({1.0, 1.0});       // set point at the slack, starting guess at the load bus
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});

  const power::Grid grid = power::make(net);
  out.writeln("buses ", grid.buses(), ", generators ", grid.gens(), ", lines ", grid.lines(), ", base power (W) ", grid.base_power());
  const auto re = flat(grid.ybus().real()), im = flat(grid.ybus().imag());
  out.writeln("Y[0][0] from the grid: ", re[0], " ", im[0], "j   Y[0][1]: ", re[1], " ", im[1], "j");
  const std::complex<double> y = 1.0 / std::complex<double>(0.01, 0.1);
  out.writeln("1 / (r + jx) by hand:  ", y.real(), " ", y.imag(), "j");
}
```

Output

```text
buses 2, generators 0, lines 1, base power (W) 100000000.000000
Y[0][0] from the grid: 0.990099 -9.900990j   Y[0][1]: -0.990099 9.900990j
1 / (r + jx) by hand:  0.990099 -9.900990j
```

The grid's admittance entry equals `1 / (r + jx)`, and the off-diagonal entry is its negative. The load bus's `vm` and `va` are only the starting guess for the iteration, because a `PQ` bus has its voltage solved for.

## Generators and voltage-controlled buses

A generator bus holds its voltage magnitude and injects a chosen amount of active power. The network below has three buses: a slack at `1.04` per unit, a generator bus at `1.02` per unit with a `0.2` per unit load and a `0.6` per unit generator, so a net injection of `0.4`, and a load bus drawing `0.5 + 0.2j`. Three lines join them, with charging susceptance on each. The program solves it and prints the net injections, which show how `p` of the bus and `p` of the generator add, and the voltage magnitudes, where the `PV` bus sits exactly at its set point.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});      // the load at bus 1 is part of the bus; its generator is listed separately
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.gens.active = {true};
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});

  const auto r = power::ac(power::make(net));
  out.writeln("voltage magnitudes: ", flat(r.magnitude), "   angles (rad): ", flat(r.angle));
  out.writeln("net active injections: ", flat(r.injection.real()), "   (bus 1: -0.2 load + 0.6 generator = 0.4)");
  out.writeln("net reactive injections: ", flat(r.injection.imag()));
  out.writeln("converged ", r.converged.item<bool>(), " in ", r.iterations, " iterations");
}
```

Output

```text
voltage magnitudes: [1.04000, 1.02000, 1.00575]   angles (rad): [0.00000, 0.01136, -0.04632]
net active injections: [0.10314, 0.40000, -0.50000]   (bus 1: -0.2 load + 0.6 generator = 0.4)
net reactive injections: [0.32097, -0.19852, -0.20000]
converged true in 3 iterations
```

The `PV` bus holds `1.02` exactly, and its net active injection is `0.4`; the slack bus supplies the rest of the load plus the losses, `0.10314`, which is how the slack absorbs the losses. A generator whose `active` entry is false is ignored; the next section shows it.

## Line parameters: charging, transformers and shunts

A line is a series impedance `r + jx` with, optionally, half its total charging susceptance `charge` at each end. A transformer is a line with an off-nominal `tap` ratio (magnitude) and a phase `shift`. A bus may carry a **shunt**, a fixed conductance `g` and susceptance `b` to ground, as from a capacitor bank. Each of these changes the admittance matrix, and the program shows what each does to the voltage at the load bus of the two-bus network above: the plain line, a tap of `1.05` on the line, a charging susceptance of `0.4`, and a shunt capacitor of `0.3` per unit at the load bus.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static power::Net two_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.5});
  net.buses.q = vec({0.0, -0.2});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto voltage = [&](const power::Net &net) { return flat(power::ac(power::make(net)).magnitude)[1]; };

  out.writeln("plain line:                    load-bus voltage ", voltage(two_bus()));
  power::Net tapped = two_bus();
  tapped.lines.tap = vec({1.05});
  out.writeln("tap ratio 1.05:                load-bus voltage ", voltage(tapped));
  power::Net charged = two_bus();
  charged.lines.charge = vec({0.4});
  out.writeln("line charging 0.4:             load-bus voltage ", voltage(charged));
  power::Net shunted = two_bus();
  shunted.buses.b = vec({0.0, 0.3});
  out.writeln("shunt capacitor 0.3 at bus 1:  load-bus voltage ", voltage(shunted));
}
```

Output

```text
plain line:                    load-bus voltage 0.97309
tap ratio 1.05:                load-bus voltage 0.92390
line charging 0.4:             load-bus voltage 0.99344
shunt capacitor 0.3 at bus 1:  load-bus voltage 1.00392
```

The tap ratio divides the voltage that appears at the load end, so a tap of `1.05` lowers the load voltage from `0.97309` to `0.92390`; line charging and a shunt capacitor both supply reactive power near the load, and both raise it, to `0.99344` and `1.00392`. The sign convention of the shunt is that a positive susceptance is capacitive.

## Generators out of service, shunt conductance and phase shift

Three more fields complete the description. A generator whose `active` entry is false contributes nothing, so a unit is taken out of service without removing it from the list. A bus shunt has a conductance `g` as well as a susceptance, a real power drawn by the bus at unit voltage, as from a resistive load or a loss. A line has a phase `shift`, which is what a phase-shifting transformer provides, and moves active power from one path to another. The program solves the three-bus network four times: as it stands, with the generator out of service, with a conductance of `0.2` per unit at the load bus, and with a phase shift of `0.05` rad on the line from bus `0` to bus `1`. For each it prints the output of the slack and the active flow on that line.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static power::Net three_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto report = [&](const char *label, const power::Net &net) {
    const auto r = power::ac(power::make(net));
    out.writeln(label, ": slack output (p.u.) ", flat(r.injection.real())[0], ", flow on line 0-1 (p.u.) ", flat(r.from_power.real())[0], ", load-bus voltage ", flat(r.magnitude)[2]);
  };
  report("as described              ", three_bus());

  power::Net unit_out = three_bus();
  unit_out.gens.active = {false};              // the generator at bus 1 is out of service
  report("generator out of service  ", unit_out);

  power::Net conductance = three_bus();
  conductance.buses.g = vec({0.0, 0.0, 0.2});  // a resistive shunt at the load bus
  report("conductance 0.2 at bus 2  ", conductance);

  power::Net shifted = three_bus();
  shifted.lines.shift = vec({0.05, 0.0, 0.0}); // a phase shifter on line 0-1
  report("phase shift 0.05 on line 0", shifted);
}
```

Output

```text
as described              : slack output (p.u.) 0.10314, flow on line 0-1 (p.u.) -0.09867, load-bus voltage 1.00575
generator out of service  : slack output (p.u.) 0.70407, flow on line 0-1 (p.u.) 0.39463, load-bus voltage 1.00609
conductance 0.2 at bus 2  : slack output (p.u.) 0.30611, flow on line 0-1 (p.u.) -0.00692, load-bus voltage 1.00250
phase shift 0.05 on line 0: slack output (p.u.) 0.10317, flow on line 0-1 (p.u.) -0.19275, load-bus voltage 1.00608
```

With the generator out the slack has to supply everything, its output rising from `0.10314` to about `0.6`, and the generator bus, now a plain `PV` bus with no generation, injects only its own load. The conductance is an extra load of about `0.2` per unit at unit voltage, which the slack also supplies. The phase shift pushes active power along the line it is on, changing the flow on line `0-1` while the slack output hardly changes: a phase shifter controls where power flows, not how much is needed.

## Taking lines out and sweeping scenarios

`active` marks each line in or out of service without rebuilding the description, and every `Series` in the network may carry leading batch axes, so one `make` and one `ac` handle many scenarios. The program takes the line between buses `0` and `2` out of the three-bus network of the generator example and compares the voltage at bus `2`, and then solves three load levels at the load bus of the two-bus network in a single call, with a leading axis of three on the injections.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static power::Net three_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));

  const auto base = power::ac(power::make(three_bus()));
  power::Net outage = three_bus();
  outage.lines.active = {true, true, false};   // line 0 - 2 out of service
  const auto cut = power::ac(power::make(outage));
  out.writeln("all lines in:         load-bus voltage ", flat(base.magnitude)[2], ", slack output (p.u.) ", flat(base.injection.real())[0]);
  out.writeln("line 0-2 out:         load-bus voltage ", flat(cut.magnitude)[2], ", slack output (p.u.) ", flat(cut.injection.real())[0]);

  // three load levels in one call: a leading axis of three on the injections
  power::Net sweep;
  sweep.base = {100e6, 230e3};
  sweep.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  sweep.buses.p = pluss::series::make(std::vector<double>{0, -0.2, 0, -0.5, 0, -1.0}).reshape({3, 2});
  sweep.buses.q = pluss::series::make(std::vector<double>{0, -0.08, 0, -0.2, 0, -0.4}).reshape({3, 2});
  sweep.buses.vm = vec({1.0, 1.0});
  sweep.buses.va = vec({0.0, 0.0});
  sweep.lines.from = {0};
  sweep.lines.to = {1};
  sweep.lines.r = vec({0.01});
  sweep.lines.x = vec({0.1});
  const power::Grid grid = power::make(sweep);
  const auto r = power::ac(grid);
  out.writeln("batch shape of the grid: ", grid.batch_shape(), "; magnitude shape ", r.magnitude.sizes());
  out.writeln("load-bus voltage at 0.2, 0.5 and 1.0 per unit of load: ", flat(r.magnitude.select(-1, 1)));
}
```

Output

```text
all lines in:         load-bus voltage 1.00575, slack output (p.u.) 0.10314
line 0-2 out:         load-bus voltage 0.96833, slack output (p.u.) 0.10646
batch shape of the grid: [3]; magnitude shape [3, 2]
load-bus voltage at 0.2, 0.5 and 1.0 per unit of load: [0.98971, 0.97309, 0.94169]
```

Taking the direct line out forces all the power through the longer path, so the load-bus voltage falls and the slack supplies more. The three load levels give three results, `[3]` per bus, with the voltage falling as the load grows, and the whole sweep cost one call.

## Decisions

- **Choosing bases:** one `Base` for the whole study, and per-unit values throughout; convert the inputs once with `pu` and the outputs back with `si`.
- **Which bus kind:** `Slack` for the single reference, `PV` for any bus whose voltage is held by a generator, `PQ` for loads and for generators without voltage control.
- **Scenarios:** `active` to switch lines or generators, and leading axes on the injections or line values for sweeps; never rebuild the network in a loop.

## Pitfalls

Each mistake here is one a first network description tends to contain. The program makes six: no slack bus, two slack buses, a line out of service that isolates a bus, a line of zero impedance, a generator on a bus that does not exist, and a power given in watts on a per-unit network. The last is not an error, which is the reason to look at it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static power::Net two_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.5});
  net.buses.q = vec({0.0, -0.2});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  return net;
}

static void report(const char *what, const std::function<void()> &build) {
  try {
    build();
    std::cout << what << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << what << ": " << e.what() << "\n";
  }
}

int main() {
  report("no slack bus          ", [] { auto n = two_bus(); n.buses.kind = {power::BusKind::PQ, power::BusKind::PQ}; (void)power::make(n); });
  report("two slack buses       ", [] { auto n = two_bus(); n.buses.kind = {power::BusKind::Slack, power::BusKind::Slack}; (void)power::make(n); });
  report("an isolated bus       ", [] { auto n = two_bus(); n.lines.active = {false}; (void)power::make(n); });
  report("zero line impedance   ", [] { auto n = two_bus(); n.lines.r = vec({0.0}); n.lines.x = vec({0.0}); (void)power::make(n); });
  report("generator off the grid", [] { auto n = two_bus(); n.gens.bus = {7}; n.gens.p = vec({0.1}); n.gens.q = vec({0.0}); (void)power::make(n); });

  // a 50 MW load written as 50 where per unit was meant: accepted, and the answer is nonsense
  power::Net watts = two_bus();
  watts.buses.p = vec({0.0, -50.0});
  watts.buses.q = vec({0.0, -20.0});
  const auto r = power::ac(power::make(watts));
  std::cout << "a load of 50 per unit (meant 0.5): accepted, converged " << r.converged.item<bool>() << ", load-bus voltage " << flat(r.magnitude)[1] << "\n";
}
```

Output

```text
no slack bus          : eng::power::make: exactly one slack bus is required
two slack buses       : eng::power::make: exactly one slack bus is required
an isolated bus       : eng::power::make: every bus must connect to the slack island
zero line impedance   : eng::power::make: line series impedance cannot be zero
generator off the grid: eng::power::make: generator bus lies outside the network
a load of 50 per unit (meant 0.5): accepted, converged 0, load-bus voltage -4.45537
```

The five structural mistakes are refused with an error that names the missing or wrong piece. The units mistake is not refused: the network is valid, and a load of fifty times the base cannot be served, so the solver reports that it did not converge, with a load-bus voltage (`4.48`) that is meaningless, rather than naming the cause. A solved network that does not converge is the first thing to check against a mistake in the per-unit conversion.

Reference: [`make`](../../../../api/eng/power/model/make.md), [`Net`](../../../../api/eng/power/model/Net.md), [`Grid`](../../../../api/eng/power/model/Grid.md), [`Base`](../../../../api/eng/power/model/Base.md), [`pu`](../../../../api/eng/power/model/pu.md), [`ac`](../../../../api/eng/power/flow/ac.md).

Where next: [DC power flow](dc_power_flow.md) solves the same networks with the linear approximation that most contingency and market studies rely on.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="dc_power_flow.html">
    <span class="label">Next</span>
    <span class="title">DC power flow</span>
  </a>
</div>
<!-- apiref:nav:end -->
