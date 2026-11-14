<!-- generated from Exchange.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Exchange`

The result of rating a heat exchanger.

## Signature

```cpp
thermal::Exchange
```

## What It Does

Holds the outputs of `rate`, each with the broadcast shape of the inputs.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `duty` | The heat in W transferred from the hot stream to the cold stream. |  |
| `hot_out` | The outlet temperature of the hot stream. |  |
| `cold_out` | The outlet temperature of the cold stream. |  |
| `effectiveness` | The effectiveness of the exchanger. |  |
| `ntu` | The number of transfer units `UA / Cmin`. |  |

## Returns

A value returned by `rate`.

## Errors

None: a result is only constructed by a successful call.

## Interpretation and Use

The energy balance holds exactly: `hot_c (hot_in - hot_out) = cold_c (cold_out - cold_in) = duty`. The duty is negative when the stream called hot is the colder one.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A rated counter-flow exchanger.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static double one(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>()[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Hot water (1000 W/K) at 90 against cold water (2000 W/K) at 20 in a counter-flow exchanger with UA = 1500 W/K.
  const auto r = th::rate(vec({1000.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({1500.0}), th::Flow::Counter);
  out.writeln("NTU", flat(r.ntu), " effectiveness", flat(r.effectiveness));
  out.writeln("duty", flat(r.duty), "W");
  out.writeln("hot out", flat(r.hot_out), " cold out", flat(r.cold_out));
  out.writeln("balance: hot gives", 1000.0 * (90.0 - one(r.hot_out)), " cold takes", 2000.0 * (one(r.cold_out) - 20.0));
}
```

Output

```text
NTU [1.50000]  effectiveness [0.69079]
duty [48354.97858] W
hot out [41.64502]  cold out [44.17749]
balance: hot gives 48354.97858  cold takes 48354.97858
```
### Example 2

A sweep of UA.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A sweep of the conductance UA: the duty saturates at Cmin (T_hot - T_cold).
  const auto r = th::rate(vec({1000.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({100.0, 500.0, 1500.0, 5000.0, 50000.0}), th::Flow::Counter);
  out.writeln("duty", flat(r.duty), "W");
  out.writeln("ceiling Cmin dT =", 1000.0 * 70.0, "W");
  out.writeln("hot out", flat(r.hot_out));
}
```

Output

```text
duty [6510.36626, 25358.59010, 48354.97858, 67004.06436, 70000.00000] W
ceiling Cmin dT = 70000.00000 W
hot out [83.48963, 64.64141, 41.64502, 22.99594, 20.00000]
```
### Example 3

A boiler.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A boiler: the cold side changes phase at a fixed temperature, so only the hot stream cools and cr is zero.
  const auto r = th::rate(vec({800.0}), vec({1.0e9}), vec({300.0}), vec({100.0}), vec({400.0}), th::Flow::Boil);
  out.writeln("effectiveness", flat(r.effectiveness), " closed form", 1.0 - std::exp(-0.5));
  out.writeln("duty", flat(r.duty), "W  hot out", flat(r.hot_out), " cold out", flat(r.cold_out));
}
```

Output

```text
effectiveness [0.39347]  closed form 0.39347
duty [62955.09445] W  hot out [221.30613]  cold out [100.00006]
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ntu.html">
    <span class="label">Previous</span>
    <span class="title">ntu</span>
  </a>
  <a class="next-link" href="rate.html">
    <span class="label">Next</span>
    <span class="title">rate</span>
  </a>
</div>
<!-- apiref:nav:end -->
