<!-- generated from ntu.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::ntu`

The transfer units a heat exchanger needs.

## Signature

```cpp
Series thermal::ntu(const Series& effectiveness,
                     const Series& cr,
                     thermal::Flow flow);
```

## What It Does

Inverts `eff`: returns the number of transfer units that gives the required effectiveness. Parallel, counter, shell-and-tube and boiling arrangements have closed-form inverses; cross flow is found by a bracketed root search over the whole batch. An effectiveness that the arrangement cannot reach is an error, not a large number.

## Parameters

`effectiveness`
: The required effectiveness, not negative and below the limit of the arrangement.

`cr`
: The capacity ratio `Cmin / Cmax` in `[0, 1]`.

`flow`
: The flow arrangement.

## Returns

The number of transfer units with the broadcast shape of the inputs; `UA = NTU Cmin`.

## Errors

Throws `pluss::Error` for an input that is not a real floating array or not finite, a negative effectiveness, a `cr` outside `[0, 1]`, an effectiveness at or above 1, and an effectiveness above the limit of the arrangement: `1 / (1 + cr)` for parallel flow and `2 / (1 + cr + sqrt(1 + cr^2))` for shell-and-tube.

## Interpretation and Use

Use it to size an exchanger: pick the effectiveness you need, get `NTU`, multiply by `Cmin` for `UA`, then divide by `U` for the area.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Sizing for a required effectiveness, in four arrangements.

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
  // ntu inverts eff: size the exchanger for a required effectiveness of 0.8.
  const pluss::Series cr = vec({0.5});
  for (const auto flow : {th::Flow::Counter, th::Flow::Parallel, th::Flow::Shell, th::Flow::Cross}) {
    const char *name = flow == th::Flow::Counter ? "counter " : flow == th::Flow::Parallel ? "parallel" : flow == th::Flow::Shell ? "shell   " : "cross   ";
    const double target = flow == th::Flow::Parallel ? 0.6 : flow == th::Flow::Shell ? 0.7 : 0.8;
    const pluss::Series n = th::ntu(vec({target}), cr, flow);
    out.writeln(name, "effectiveness", target, " needs NTU", flat(n), " check eff =", flat(th::eff(n, cr, flow)));
  }
}
```

Output

```text
counter  effectiveness 0.80000  needs NTU [2.19722]  check eff = [0.80000]
parallel effectiveness 0.60000  needs NTU [1.53506]  check eff = [0.60000]
shell    effectiveness 0.70000  needs NTU [2.09041]  check eff = [0.70000]
cross    effectiveness 0.80000  needs NTU [2.60518]  check eff = [0.80000]
```
### Example 2

The limits of the arrangements.

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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  // Some arrangements cannot reach every effectiveness; the call says so rather than returning a nonsense NTU.
  const pluss::Series cr = vec({0.5});
  out.writeln("shell-and-tube ceiling at cr = 0.5:", 2.0 / (1.5 + std::sqrt(1.25)));
  out.writeln("parallel-flow ceiling at cr = 0.5:", 1.0 / 1.5);
  report([&] { (void)th::ntu(vec({0.8}), cr, th::Flow::Shell); });
  report([&] { (void)th::ntu(vec({0.7}), cr, th::Flow::Parallel); });
  report([&] { (void)th::ntu(vec({1.0}), cr, th::Flow::Counter); });
  out.writeln("counter flow can approach 1:", flat(th::ntu(vec({0.999}), cr, th::Flow::Counter)), "units");
}
```

Output

```text
shell-and-tube ceiling at cr = 0.5: 0.76393
parallel-flow ceiling at cr = 0.5: 0.66667
eng::thermal::ntu: effectiveness is at or above the limit 2 / (1 + cr + sqrt(1 + cr^2)) of a shell-and-tube exchanger
eng::thermal::ntu: a parallel-flow exchanger cannot exceed effectiveness 1 / (1 + cr)
eng::thermal::ntu: a counter-flow exchanger cannot reach effectiveness 1
counter flow can approach 1: [12.43122] units
```
### Example 3

Cross flow by root search.

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
  // Cross flow has no closed-form inverse, so it is found by root bracketing; the batch is solved together.
  const pluss::Series target = vec({0.2, 0.5, 0.7, 0.9});
  const pluss::Series n = th::ntu(target, vec({0.75}), th::Flow::Cross);
  out.writeln("NTU needed", flat(n));
  out.writeln("effectiveness back", flat(th::eff(n, vec({0.75}), th::Flow::Cross)));
}
```

Output

```text
NTU needed [0.25301, 0.98029, 2.24559, 12.25852]
effectiveness back [0.20000, 0.50000, 0.70000, 0.90000]
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
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

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
  report([&] { (void)th::ntu(vec({-0.1}), vec({0.5}), th::Flow::Counter); });  // negative effectiveness
  report([&] { (void)th::ntu(vec({0.5}), vec({2.0}), th::Flow::Counter); });   // cr above one
  report([&] { (void)th::ntu(vec({1.5}), vec({0.5}), th::Flow::Cross); });     // effectiveness above one
  out.writeln("a boiler needs", flat(th::ntu(vec({0.5}), vec({0.5}), th::Flow::Boil)), "units for 0.5");
}
```

Output

```text
eng::thermal::ntu: effectiveness must not be negative
eng::thermal::ntu: cr must lie in [0, 1]
eng::thermal::ntu: effectiveness must be below 1
a boiler needs [0.69315] units for 0.5
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eff.html">
    <span class="label">Previous</span>
    <span class="title">eff</span>
  </a>
  <a class="next-link" href="Exchange.html">
    <span class="label">Next</span>
    <span class="title">Exchange</span>
  </a>
</div>
<!-- apiref:nav:end -->
