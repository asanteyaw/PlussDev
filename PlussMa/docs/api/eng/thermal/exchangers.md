<!-- generated from exchangers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Heat exchangers

The effectiveness-NTU method for five flow arrangements, its inverse for sizing, a rating that returns duty and outlet temperatures, and the log-mean temperature difference.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

All arrangements at one operating point.

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
### Example 3

Sizing for a required effectiveness.

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
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

exchangers/Flow
exchangers/eff
exchangers/ntu
exchangers/Exchange
exchangers/rate
exchangers/lmtd
```
<!-- apiref:toctree:end -->
