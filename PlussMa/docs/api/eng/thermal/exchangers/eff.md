<!-- generated from eff.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::eff`

The effectiveness of a heat exchanger.

## Signature

```cpp
Series thermal::eff(const Series& ntu,
                     const Series& cr,
                     thermal::Flow flow);
```

## What It Does

Returns the fraction of the largest possible heat transfer that the exchanger achieves, given `ntu = UA / Cmin` and the capacity ratio `cr = Cmin / Cmax`. Every flow arrangement has a closed form (see `Flow`). The inputs broadcast.

## Parameters

`ntu`
: The number of transfer units, not negative.

`cr`
: The capacity ratio `Cmin / Cmax` in `[0, 1]`.

`flow`
: The flow arrangement.

## Returns

The effectiveness in `[0, 1)` with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for an input that is not a real floating array or not finite, a negative `ntu`, or a `cr` outside `[0, 1]`.

## Interpretation and Use

Balanced counter flow (`cr = 1`) is evaluated without a special case, to about `1e-8` relative accuracy. Cross flow is an approximation of the exact series solution and is the least precise of the five.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

All five arrangements.

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

Balanced counter flow and its neighbours.

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
  // Balanced counter flow (cr = 1) has the closed form NTU / (1 + NTU); the limit is smooth, not a special case.
  out.writeln("cr = 1      ", flat(th::eff(vec({0.5, 1.0, 3.0, 10.0}), vec({1.0}), th::Flow::Counter)), " closed form", 0.5 / 1.5, 1.0 / 2.0, 3.0 / 4.0, 10.0 / 11.0);
  out.writeln("cr = 0.9999 ", flat(th::eff(vec({0.5, 1.0, 3.0, 10.0}), vec({0.9999}), th::Flow::Counter)));
  out.writeln("cr = 0      ", flat(th::eff(vec({0.5, 1.0, 3.0, 10.0}), vec({0.0}), th::Flow::Counter)), " all arrangements agree here: 1 - exp(-NTU)");
}
```

Output

```text
cr = 1       [0.33333, 0.50000, 0.75000, 0.90909]  closed form 0.33333 0.50000 0.75000 0.90909
cr = 0.9999  [0.33334, 0.50001, 0.75003, 0.90913]
cr = 0       [0.39347, 0.63212, 0.95021, 0.99995]  all arrangements agree here: 1 - exp(-NTU)
```
### Example 3

A grid of NTU against capacity ratio.

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
  // A grid of NTU (rows) against capacity ratio (columns) for counter flow in one call.
  const pluss::Series ntu = vec({0.5, 1.0, 2.0, 4.0}).reshape({4, 1});
  const pluss::Series cr = vec({0.0, 0.5, 1.0}).reshape({1, 3});
  const pluss::Series e = th::eff(ntu, cr, th::Flow::Counter);
  out.writeln("shape", e.sizes(), " (ntu, cr)");
  for (int row = 0; row < 4; ++row) out.writeln("ntu", flat(ntu)[row], flat(e.select(0, row)));
}
```

Output

```text
shape [4, 3]  (ntu, cr)
ntu 0.50000 [0.39347, 0.36227, 0.33333]
ntu 1.00000 [0.63212, 0.56473, 0.50000]
ntu 2.00000 [0.86466, 0.77460, 0.66667]
ntu 4.00000 [0.98168, 0.92742, 0.80000]
```
### Example 4

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
  report([&] { (void)th::eff(vec({-1.0}), vec({0.5}), th::Flow::Counter); });   // negative NTU
  report([&] { (void)th::eff(vec({2.0}), vec({1.5}), th::Flow::Counter); });    // cr above one: it is Cmin / Cmax
  report([&] { (void)th::eff(vec({2.0}), vec({-0.1}), th::Flow::Counter); });   // negative cr
  report([&] { (void)th::eff(vec({std::nan("")}), vec({0.5}), th::Flow::Cross); }); // not finite
}
```

Output

```text
eng::thermal::eff: ntu must not be negative
eng::thermal::eff: cr must lie in [0, 1]
eng::thermal::eff: cr must lie in [0, 1]
eng::thermal::eff: ntu must be finite
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Flow.html">
    <span class="label">Previous</span>
    <span class="title">Flow</span>
  </a>
  <a class="next-link" href="ntu.html">
    <span class="label">Next</span>
    <span class="title">ntu</span>
  </a>
</div>
<!-- apiref:nav:end -->
