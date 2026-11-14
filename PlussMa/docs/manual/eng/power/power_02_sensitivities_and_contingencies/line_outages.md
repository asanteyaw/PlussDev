<!-- generated from line_outages.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Line outages

[Back to Power 2: Sensitivities and Contingencies](../power_02_sensitivities_and_contingencies.md)

A network must survive the loss of any one line without overloading another, the N-1 criterion. Checking it means solving the network once for every possible outage. The DC model makes that a single matrix computation: the **line outage distribution factors** give the new flow on every line for the loss of any one. `power::lodf` returns the factors, and `power::outages` applies them to the base flows and returns the flows after every outage. This page explains the factors, checks them against a re-solved network, screens a five-bus network for overloads, handles the lines whose loss splits the network, and compares the DC answer with the AC one.

## The mental model

When line `k` is lost, the flow it carried, `F_k`, has to go somewhere, and it redistributes over the remaining lines. The share that goes to monitored line `m` is its **LODF** for outage `k`, so that

```{math}
F_m^{\text{after}} = F_m + \mathrm{LODF}_{m,k} \, F_k .
```

`lodf(grid, tolerance)` returns a `LodfResult`:

| Member | Shape | Meaning |
|---|---|---|
| `factor` | `[..., monitored line, outage line]` | the LODF; the entry for a line monitored against its own outage is `-1`, since the line carries nothing afterwards |
| `valid` | `[..., outage line]` | false for an outage that is not a single-line contingency of a connected network |

`outages(grid, tolerance)` applies the factors to the base DC flows and returns an `OutageResult`:

| Member | Shape | Meaning |
|---|---|---|
| `base_flow` | `[..., line]` | the DC flows before any outage |
| `flow` | `[..., outage line, monitored line]` | the flows after each outage; the row of an invalid outage is `NaN` |
| `valid` | `[..., outage line]` | as in `LodfResult` |

An outage is **invalid** when the line is out of service already, or when losing it would split the network: the factors would divide by zero, since all of the flow cannot be redistributed. `tolerance` is the smallest denominator, `|1 - PTDF-of-the-line-against-itself|`, that is accepted.

## Factors of a ring, and the check

In the ring of three buses every line is on a loop, and losing one sends its whole flow round the other two. At the base case each line carries `0.25`. Losing line `2` (the direct line from `0` to `2`) puts the load of `0.5` on the path through bus `1`, so lines `0` and `1` carry `0.5`, which is `0.25 + 1 * 0.25`: the LODF of lines `0` and `1` for the outage of line `2` is `1`. The program prints the factors and the flows after each outage, and then checks one outage directly, by removing the line from service and solving the DC power flow of the network that remains.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: the slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 between them.
// Lines: 0 joins buses 0-1 (x = 0.1), line 1 joins 1-2 (x = 0.2), line 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto factors = power::lodf(grid);
  const auto f = flat(factors.factor);
  out.writeln("LODF [monitored line][outage line], shape ", factors.factor.sizes(), ":");
  for (int m = 0; m < 3; ++m) out.writeln("  line ", m, ": ", f[3 * m], " ", f[3 * m + 1], " ", f[3 * m + 2]);
  out.writeln("valid outages: ", flat(factors.valid));

  const auto result = power::outages(grid);
  const auto after = flat(result.flow);
  out.writeln("base flows: ", flat(result.base_flow));
  for (int k = 0; k < 3; ++k) out.writeln("flows after losing line ", k, ": ", after[3 * k], " ", after[3 * k + 1], " ", after[3 * k + 2]);

  // the same outage by taking line 2 out of service and solving again
  power::Net direct = ring();
  direct.lines.active = {true, true, false};
  out.writeln("flows from a new DC solve without line 2: ", flat(power::dc(power::make(direct)).from_power));
}
```

Output

```text
LODF [monitored line][outage line], shape [3, 3]:
  line 0: -1.00000 -1.00000 1.00000
  line 1: -1.00000 -1.00000 1.00000
  line 2: 1.00000 1.00000 -1.00000
valid outages: [1.00000, 1.00000, 1.00000]
base flows: [0.25000, 0.25000, 0.25000]
flows after losing line 0: 0.00000 0.00000 0.50000
flows after losing line 1: 0.00000 0.00000 0.50000
flows after losing line 2: 0.50000 0.50000 0.00000
flows from a new DC solve without line 2: [0.50000, 0.50000, 0.00000]
```

The flows after the loss of line `2` are `0.5, 0.5, 0`, equal to the new solve without the line: the whole load goes through lines `0` and `1`. Losing line `0` or line `1` sends the whole load through line `2` instead, and the other line of the pair is left carrying nothing. The factor of a line against its own outage is `-1`: it loses all its flow. Every other factor in this ring is `+1` or `-1`, because the ring has only two paths between the loaded bus and the slack.

## Screening a network for overloads

The five-bus network is meshed, and the base flows are within the ratings, but an outage can overload a neighbour. The program gives each line a rating, a power transfer limit in per unit, and checks every outage at once: `outages` gives a matrix of post-outage flows `[outage, monitored line]`, a comparison with the ratings is one array operation, and the result lists every outage and every line it overloads. It also prints the base flows and the worst loading.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }


// five buses: a slack generator at bus 0, a second generator of 4 per unit at bus 3, and loads of 3, 3 and 4 per unit at buses 1, 2 and 4.
// Lines: 0: 0-1, 1: 0-3, 2: 0-4, 3: 1-2, 4: 2-3, 5: 3-4.
static power::Net five_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -3.0, -3.0, 0.0, -4.0});
  net.buses.q = vec({0.0, -0.6, -0.6, 0.0, -0.8});
  net.buses.vm = vec({1.0, 1.0, 1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.gens.bus = {3};
  net.gens.p = vec({4.0});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 0, 0, 1, 2, 3};
  net.lines.to = {1, 3, 4, 2, 3, 4};
  net.lines.x = vec({0.028, 0.030, 0.006, 0.011, 0.030, 0.030});
  net.lines.r = vec({0.0028, 0.003, 0.0006, 0.0011, 0.003, 0.003});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const power::Grid grid = power::make(five_bus());
  const auto result = power::outages(grid);
  const std::vector<double> rating = {4.0, 3.0, 4.5, 4.0, 3.5, 4.1};
  const auto base = flat(result.base_flow);
  const auto after = flat(result.flow);

  out.writeln("base flows:  ", base);
  out.writeln("ratings:     ", rating);
  int violations = 0;
  for (int k = 0; k < 6; ++k)
    for (int m = 0; m < 6; ++m)
      if (m != k && std::abs(after[6 * k + m]) > rating[m]) {
        ++violations;
        out.writeln("losing line ", k, " overloads line ", m, ": flow ", after[6 * k + m], " against a rating of ", rating[m]);
      }
  out.writeln(violations, " overloads in all");
}
```

Output

```text
base flows:  [3.006, -0.185, 3.179, 0.006, -2.994, 0.821]
ratings:     [4.000, 3.000, 4.500, 4.000, 3.500, 4.100]
losing line 0 overloads line 2: flow 4.545 against a rating of 4.500
losing line 0 overloads line 4: flow -6.000 against a rating of 3.500
losing line 4 overloads line 0: flow 6.000 against a rating of 4.000
3 overloads in all
```

Every outage and every monitored line is checked in one pass over a `6 x 6` matrix; no network was solved again. The overloads it lists are the lines an operator would have to relieve, by redispatching generation or shedding load, to stay N-1 secure; a later chapter on dispatch does that automatically.

## When the outage splits the network

A line whose loss isolates a bus, such as the only line to a spur, cannot be redistributed, and the factors for it do not exist. The program adds a bus `3` hanging off bus `2` by a line, and prints the factors, the validity and the post-outage flows. The invalid outage appears as `NaN` rather than as a number, and `tolerance` is the threshold below which a denominator is treated as zero; the program also shows that a large tolerance declares every outage invalid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: the slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 between them.
// Lines: 0 joins buses 0-1 (x = 0.1), line 1 joins 1-2 (x = 0.2), line 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  power::Net spur = ring();
  spur.buses.kind.push_back(power::BusKind::PQ);   // a fourth bus on a spur from bus 2
  spur.buses.p = vec({0.0, 0.0, -0.5, -0.1});
  spur.buses.q = vec({0.0, 0.0, -0.1, -0.02});
  spur.buses.vm = vec({1.0, 1.0, 1.0, 1.0});
  spur.buses.va = vec({0.0, 0.0, 0.0, 0.0});
  spur.lines.from.push_back(2);
  spur.lines.to.push_back(3);
  spur.lines.r = vec({0.01, 0.02, 0.03, 0.01});
  spur.lines.x = vec({0.1, 0.2, 0.3, 0.1});
  const power::Grid grid = power::make(spur);

  const auto result = power::outages(grid);
  out.writeln("valid outages (one entry per line): ", flat(result.valid));
  const auto after = flat(result.flow);
  for (int k = 0; k < 4; ++k) out.writeln("flows after losing line ", k, ": ", after[4 * k], " ", after[4 * k + 1], " ", after[4 * k + 2], " ", after[4 * k + 3]);
  out.writeln("with tolerance 0.9, valid outages: ", flat(power::lodf(grid, 0.9).valid));
}
```

Output

```text
valid outages (one entry per line): [1.000, 1.000, 1.000, 0.000]
flows after losing line 0: 0.000 0.000 0.600 0.100
flows after losing line 1: 0.000 0.000 0.600 0.100
flows after losing line 2: 0.600 0.600 0.000 0.100
flows after losing line 3: nan nan nan nan
with tolerance 0.9, valid outages: [0.000, 0.000, 0.000, 0.000]
```

The spur line, line `3`, is flagged invalid, and every flow after its loss is `NaN`: the bus it served is gone, and there is no redistribution to compute. With the default tolerance the three loop lines are valid, and with a tolerance of `0.9` they are declared invalid too, because their denominators are below that threshold; leave the tolerance at its default unless you want to exclude lines that are nearly bridges.

## How good is DC for outages

The factors are exact for the DC model and approximate for the real network. The program compares the DC flows after losing line `2` of the ring with an AC power flow of the same network with the line out of service.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: the slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 between them.
// Lines: 0 joins buses 0-1 (x = 0.1), line 1 joins 1-2 (x = 0.2), line 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto dc = flat(power::outages(grid).flow);   // [outage][monitored]
  power::Net cut = ring();
  cut.lines.active = {true, true, false};
  const auto ac = power::ac(power::make(cut));
  out.writeln("losing line 2: DC flows ", dc[6], " ", dc[7], " ", dc[8], "   AC flows ", flat(ac.from_power.real())[0], " ", flat(ac.from_power.real())[1], " ", flat(ac.from_power.real())[2]);
  out.writeln("AC converged ", ac.converged.item<bool>(), ", AC voltage at the load bus ", flat(ac.magnitude)[2]);
}
```

Output

```text
losing line 2: DC flows 0.50000 0.50000 0.00000   AC flows 0.50883 0.50589 0.00000
AC converged true, AC voltage at the load bus 0.93981
```

The DC and AC flows agree to a percent: the AC flows are `0.50883` and `0.50589` where the DC ones are `0.5`, the difference being the losses on the lines and the reactive power the approximation ignores. Use the DC factors to screen, and confirm the critical outages with a full AC power flow, as the [AC page](../power_01_networks_and_power_flow/ac_power_flow.md) describes.

## Decisions

- **Screening every single outage:** `outages`, then compare with the ratings in one array operation.
- **Only some outages or some lines:** `lodf` and index the matrix, to avoid forming flows you do not need.
- **Confirming a critical outage:** an AC power flow with the line's `active` flag false.

## Pitfalls

The program makes the mistake that `NaN` invites. It counts the overloads after every outage of the spur network by comparing each post-outage flow with the rating, without looking at `valid`. A comparison with `NaN` is false, so an outage that splits the network reports no overload, as if the contingency were harmless. The program prints the naive count and then the invalid outages that it silently passed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: the slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 between them.
// Lines: 0 joins buses 0-1 (x = 0.1), line 1 joins 1-2 (x = 0.2), line 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  power::Net spur = ring();
  spur.buses.kind.push_back(power::BusKind::PQ);
  spur.buses.p = vec({0.0, 0.0, -0.5, -0.1});
  spur.buses.q = vec({0.0, 0.0, -0.1, -0.02});
  spur.buses.vm = vec({1.0, 1.0, 1.0, 1.0});
  spur.buses.va = vec({0.0, 0.0, 0.0, 0.0});
  spur.lines.from.push_back(2);
  spur.lines.to.push_back(3);
  spur.lines.r = vec({0.01, 0.02, 0.03, 0.01});
  spur.lines.x = vec({0.1, 0.2, 0.3, 0.1});
  const auto result = power::outages(power::make(spur));
  const auto after = flat(result.flow);
  const auto valid = flat(result.valid);
  const double rating = 0.45;   // a tight rating on every line

  int naive = 0, skipped = 0;
  for (int k = 0; k < 4; ++k) {
    if (valid[k] < 0.5) ++skipped;
    for (int m = 0; m < 4; ++m)
      if (std::abs(after[4 * k + m]) > rating) ++naive;
  }
  out.writeln("overloads counted by comparing every entry: ", naive);
  out.writeln("outages that were not valid contingencies and were silently skipped: ", skipped, " (the spur line: its post-outage row is NaN, and a comparison with NaN is false)");
}
```

Output

```text
overloads counted by comparing every entry: 4
outages that were not valid contingencies and were silently skipped: 1 (the spur line: its post-outage row is NaN, and a comparison with NaN is false)
```

The count is not zero, since the loop outages overload lines, so nothing looks wrong, but the outage of the spur line is neither safe nor unsafe: the network splits, and the screening has to treat it separately, as an islanding event. Always read `valid` next to the flows.

Reference: [`lodf`](../../../../api/eng/power/security/lodf.md), [`outages`](../../../../api/eng/power/security/outages.md), [`LodfResult`](../../../../api/eng/power/security/LodfResult.md), [`OutageResult`](../../../../api/eng/power/security/OutageResult.md), [`ptdf`](../../../../api/eng/power/security/ptdf.md), [`ac`](../../../../api/eng/power/flow/ac.md).

Where next: the next chapter turns the same DC model into an optimisation, choosing generation at least cost while keeping every line within its rating, with or without N-1 security.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sensitivity_factors.html">
    <span class="label">Previous</span>
    <span class="title">Sensitivity factors</span>
  </a>
</div>
<!-- apiref:nav:end -->
