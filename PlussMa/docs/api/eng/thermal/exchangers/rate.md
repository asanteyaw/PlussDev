<!-- generated from rate.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::rate`

Rates a heat exchanger of given conductance.

## Signature

```cpp
thermal::Exchange thermal::rate(const Series& hot_c,
                             const Series& cold_c,
                             const Series& hot_in,
                             const Series& cold_in,
                             const Series& ua,
                             thermal::Flow flow);
```

## What It Does

Computes the duty and the outlet temperatures of an exchanger of conductance `ua`. The smaller capacity rate is `Cmin`, `NTU = ua / Cmin`, `cr = Cmin / Cmax`, and the duty is `eff(NTU, cr, flow) * Cmin * (hot_in - cold_in)`.

## Parameters

`hot_c`
: The capacity rate `m cp` of the hot stream in W/K, positive.

`cold_c`
: The capacity rate of the cold stream in W/K, positive.

`hot_in`
: The inlet temperature of the hot stream.

`cold_in`
: The inlet temperature of the cold stream.

`ua`
: The conductance `U A` in W/K, not negative.

`flow`
: The flow arrangement. For `Boil` give the phase-changing stream a very large capacity rate.

## Returns

An `Exchange`.

## Errors

Throws `pluss::Error` for a capacity rate that is not positive, a `ua` that is negative, or an input that is not a real floating array or not finite.

## Interpretation and Use

The duty saturates at `Cmin (hot_in - cold_in)` as `ua` grows: more area cannot beat the second law. For a given duty and approach temperature, size with `ntu` instead.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A counter-flow exchanger.

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

The same exchanger in four arrangements.

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
### Example 4

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
### Example 5

Rejected inputs.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  report([&] { (void)th::rate(vec({0.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({1500.0}), th::Flow::Counter); });   // a zero capacity rate
  report([&] { (void)th::rate(vec({1000.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({-5.0}), th::Flow::Counter); }); // negative UA
  report([&] { (void)th::rate(vec({1000.0}), vec({2000.0}), vec({std::nan("")}), vec({20.0}), vec({1500.0}), th::Flow::Counter); });
}
```

Output

```text
eng::thermal::rate: hot_c must be positive
eng::thermal::rate: ua must not be negative
eng::thermal::rate: hot_in must be finite
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Exchange.html">
    <span class="label">Previous</span>
    <span class="title">Exchange</span>
  </a>
  <a class="next-link" href="lmtd.html">
    <span class="label">Next</span>
    <span class="title">lmtd</span>
  </a>
</div>
<!-- apiref:nav:end -->
