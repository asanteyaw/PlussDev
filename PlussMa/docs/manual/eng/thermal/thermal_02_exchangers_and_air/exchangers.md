<!-- generated from exchangers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Heat exchangers

[Back to Thermal 2: Exchangers and Air](../thermal_02_exchangers_and_air.md)

Two streams pass on either side of a wall and one warms the other. You want to know what comes out, or how large the exchanger must be for a required outlet. This page rates an exchanger of given size, sizes one for a given duty, and relates both to the log-mean temperature difference.

## The mental model

The most heat a perfect, infinitely large exchanger could move is `Cmin (T_hot_in - T_cold_in)`, where `C = m cp` is the capacity rate of a stream and `Cmin` the smaller of the two. The **effectiveness** is the fraction a real exchanger achieves. It depends on only two numbers, the number of transfer units `NTU = UA / Cmin` (how big, in units of the weaker stream) and the capacity ratio `cr = Cmin / Cmax` (how unequal the streams are), and on the flow arrangement: parallel, counter, cross, shell-and-tube, or one stream boiling. `eff` evaluates the relation, `ntu` inverts it, and `rate` applies it to real streams.

## Effectiveness of the arrangements

At one operating point (`NTU = 2`, `cr = 0.5`) the five arrangements:

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
Counter flow is the best of the flow arrangements and parallel flow the worst; cross flow and shell-and-tube lie between. A boiling stream stays at one temperature, so `cr` has no effect and the effectiveness is `1 - exp(-NTU)`.

Counter flow with equal capacity rates, `cr = 1`, is the awkward limit of the textbook formula (`0 / 0`). It is evaluated without a special case:

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
At `cr = 1` the result is `NTU / (1 + NTU)`, the neighbouring `cr = 0.9999` differs by a part in ten thousand, and at `cr = 0` every arrangement gives `1 - exp(-NTU)`. A grid of `NTU` against `cr` is one call, because the inputs broadcast:

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
## Rating an exchanger

`rate` takes the capacity rates, the inlet temperatures and `UA`, and returns the duty, both outlet temperatures, the effectiveness and `NTU`. Hot water at 1000 W/K, cold water at 2000 W/K, counter flow, `UA = 1500 W/K`:

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
The cold stream has the larger capacity rate, so it changes less; the energy balance closes to rounding error. The same exchanger in four arrangements:

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
Counter flow transfers 16 % more heat than parallel flow here. Duty as a function of `UA`:

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
The duty rises quickly at first and saturates at `Cmin (T_hot - T_cold) = 70 kW`: a tenfold increase from 5000 to 50 000 W/K buys only about 4 % more. A boiler or condenser, where one stream stays at one temperature, is `Flow::Boil` with a very large capacity rate on that stream:

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
## Sizing for a required effectiveness

Sizing runs the relation the other way. Choose the effectiveness the process needs; `ntu` returns the transfer units, `UA` is `NTU Cmin`, and the area is `UA / U`.

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
For the same effectiveness a counter-flow exchanger needs 2.20 units and a cross-flow one 2.61. Parallel flow and shell-and-tube arrangements have ceilings on effectiveness; `ntu` refuses a request beyond them instead of returning a nonsense number:

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
Cross flow has no closed-form inverse; it is found by a root search over the whole batch, and the round trip through `eff` closes:

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
## Log-mean temperature difference

When you know all four terminal temperatures, the log-mean difference gives `duty = UA LMTD`. It is the mean of the end differences, weighted logarithmically, and equal end differences give their common value.

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
  // Hot 90 -> 50, cold 20 -> 60 (counter flow): the end differences are 30 and 30, so the log mean is 30.
  out.writeln("equal ends", flat(th::lmtd(vec({90.0}), vec({50.0}), vec({20.0}), vec({60.0}), th::Flow::Counter)));
  // Hot 90 -> 40, cold 20 -> 50: the end differences are 40 and 20.
  out.writeln("unequal ends", flat(th::lmtd(vec({90.0}), vec({40.0}), vec({20.0}), vec({50.0}), th::Flow::Counter)), " closed form", 20.0 / std::log(2.0));
}
```

Output

```text
equal ends [30.00000]
unequal ends [28.85390]  closed form 28.85390
```
The arrangement matters through the pairing of the ends:

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
  // The same terminal temperatures differ by arrangement: parallel flow pairs hot-in with cold-in.
  out.writeln("counter ", flat(th::lmtd(vec({90.0}), vec({50.0}), vec({20.0}), vec({40.0}), th::Flow::Counter)));
  out.writeln("parallel", flat(th::lmtd(vec({90.0}), vec({50.0}), vec({20.0}), vec({40.0}), th::Flow::Parallel)), " closed form", (70.0 - 10.0) / std::log(70.0 / 10.0));
}
```

Output

```text
counter  [39.15230]
parallel [30.83390]  closed form 30.83390
```
And the two methods agree, which is a useful check on any exchanger data:

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
  // Duty = UA x LMTD agrees with the effectiveness method for the same exchanger.
  const auto r = th::rate(vec({1000.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({1500.0}), th::Flow::Counter);
  const pluss::Series lm = th::lmtd(vec({90.0}), r.hot_out, vec({20.0}), r.cold_out, th::Flow::Counter);
  out.writeln("LMTD", flat(lm), " UA x LMTD", 1500.0 * one(lm), " duty from NTU", flat(r.duty));
}
```

Output

```text
LMTD [32.23665]  UA x LMTD 48354.97858  duty from NTU [48354.97858]
```
## Decisions

- **Rate or size:** `rate` when `UA` is known; `ntu` when the effectiveness is the requirement.
- **Effectiveness-NTU or LMTD:** use effectiveness when outlets are unknown (rating, simulation); use `lmtd` when all four temperatures are known (a measurement, a specification) and only parallel or counter flow is involved.
- **Phase change:** `Flow::Boil`, giving the boiling stream a very large capacity rate.
- **Cross flow:** the closed form is the standard approximation to the exact series and is the least precise of the five.

## Pitfalls

An effectiveness that the arrangement cannot reach, a temperature cross (the hot stream colder than the cold one at an end), and an arrangement without a plain log mean are all refused:

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
  report([&] { (void)th::lmtd(vec({50.0}), vec({30.0}), vec({40.0}), vec({45.0}), th::Flow::Counter); });   // the streams cross
  report([&] { (void)th::lmtd(vec({90.0}), vec({50.0}), vec({20.0}), vec({60.0}), th::Flow::Cross); });     // no plain log mean for cross flow
  report([&] { (void)th::lmtd(vec({90.0}), vec({50.0}), vec({std::nan("")}), vec({60.0}), th::Flow::Counter); });  // not finite
}
```

Output

```text
eng::thermal::lmtd: the hot stream must stay warmer than the cold stream at both ends (a temperature cross)
eng::thermal::lmtd: only parallel and counter flow have a plain log-mean difference; use rate for other arrangements
eng::thermal::lmtd: cold_in must be finite
```
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
## Where next

Reference pages: [`Flow`](../../../../api/eng/thermal/exchangers/Flow.md), [`eff`](../../../../api/eng/thermal/exchangers/eff.md), [`ntu`](../../../../api/eng/thermal/exchangers/ntu.md), [`rate`](../../../../api/eng/thermal/exchangers/rate.md), [`Exchange`](../../../../api/eng/thermal/exchangers/Exchange.md), [`lmtd`](../../../../api/eng/thermal/exchangers/lmtd.md). Next: [Moist air](moist_air.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="moist_air.html">
    <span class="label">Next</span>
    <span class="title">Moist air</span>
  </a>
</div>
<!-- apiref:nav:end -->
