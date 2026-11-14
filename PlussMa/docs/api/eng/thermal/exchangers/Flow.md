<!-- generated from Flow.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Flow`

The flow arrangement of a heat exchanger.

## Signature

```cpp
thermal::Flow
```

## What It Does

Selects the flow arrangement for `eff`, `ntu` and `rate`. The arrangement fixes how effectiveness depends on `NTU = UA / Cmin` and the capacity ratio `cr = Cmin / Cmax`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `Parallel` | Both streams enter at the same end. `(1 - exp(-NTU (1 + cr))) / (1 + cr)`; the least effective arrangement. |  |
| `Counter` | The streams enter at opposite ends. `(1 - e) / (1 - cr e)` with `e = exp(-NTU (1 - cr))`, and `NTU / (1 + NTU)` for `cr = 1`; the most effective. |  |
| `Cross` | Cross flow with both streams unmixed, by the standard closed-form approximation `1 - exp(NTU^0.22 (exp(-cr NTU^0.78) - 1) / cr)`. |  |
| `Shell` | One shell pass and any even number of tube passes: `2 / (1 + cr + sqrt(1 + cr^2) (1 + e) / (1 - e))` with `e = exp(-NTU sqrt(1 + cr^2))`. |  |
| `Boil` | One stream changes phase at a constant temperature, so `cr = 0` whatever the values given: `1 - exp(-NTU)`. |  |

## Returns

A choice of five arrangements, read by `eff`, `ntu` and `rate`.

## Errors

None: all values are always valid.

## Interpretation and Use

For `cr` near zero every arrangement agrees; they separate as `cr` approaches one. Counter flow beats parallel flow at the same `UA`, and shell-and-tube and cross flow lie between.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

All five arrangements at one operating point.

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
  // The five arrangements at the same NTU = 2 and capacity ratio 0.5.
  const pluss::Series ntu = vec({2.0}), cr = vec({0.5});
  out.writeln("parallel", flat(th::eff(ntu, cr, th::Flow::Parallel)), " closed form", (1.0 - std::exp(-2.0 * 1.5)) / 1.5);
  out.writeln("counter ", flat(th::eff(ntu, cr, th::Flow::Counter)), " closed form", (1.0 - std::exp(-2.0 * 0.5)) / (1.0 - 0.5 * std::exp(-2.0 * 0.5)));
  out.writeln("cross   ", flat(th::eff(ntu, cr, th::Flow::Cross)));
  out.writeln("shell   ", flat(th::eff(ntu, cr, th::Flow::Shell)));
  out.writeln("boil    ", flat(th::eff(ntu, cr, th::Flow::Boil)), " closed form", 1.0 - std::exp(-2.0), " (cr plays no part)");
}
```

Output

```text
parallel [0.63348]  closed form 0.63348
counter  [0.77460]  closed form 0.77460
cross    [0.73876]
shell    [0.69309]
boil     [0.86466]  closed form 0.86466  (cr plays no part)
```
### Example 2

A rated exchanger in every arrangement.

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
  // The same duty question in each arrangement: counter flow does the most, parallel the least.
  for (const auto flow : {th::Flow::Parallel, th::Flow::Cross, th::Flow::Shell, th::Flow::Counter}) {
    const auto r = th::rate(vec({1000.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({1500.0}), flow);
    const char *name = flow == th::Flow::Parallel ? "parallel" : flow == th::Flow::Cross ? "cross   " : flow == th::Flow::Shell ? "shell   " : "counter ";
    out.writeln(name, "duty", flat(r.duty), "W  hot out", flat(r.hot_out), " cold out", flat(r.cold_out));
  }
}
```

Output

```text
parallel duty [41748.03619] W  hot out [48.25196]  cold out [40.87402]
cross    duty [46357.62817] W  hot out [43.64237]  cold out [43.17881]
shell    duty [44698.42487] W  hot out [45.30158]  cold out [42.34921]
counter  duty [48354.97858] W  hot out [41.64502]  cold out [44.17749]
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
  <a class="next-link" href="eff.html">
    <span class="label">Next</span>
    <span class="title">eff</span>
  </a>
</div>
<!-- apiref:nav:end -->
