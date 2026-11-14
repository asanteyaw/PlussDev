<!-- generated from FinResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::FinResult`

The heat, efficiency and tip temperature of a fin.

## Signature

```cpp
thermal::FinResult
```

## What It Does

Holds the three outputs of `fin`, each with the broadcast shape of the inputs.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `heat` | The heat in W that leaves the base of the fin. |  |
| `efficiency` | The heat divided by the heat of an isothermal fin at the base temperature, `h A_surface (T_base - T_ambient)`. A convective tip adds the tip area to the surface. For an infinite fin the length does not enter the heat, but the efficiency is still measured against an isothermal fin of the stated length, so it exceeds one when that length is short. |  |
| `tip` | The temperature at the tip; for an infinite fin the ambient temperature. |  |

## Returns

A value returned by `fin`.

## Errors

None: a result is only constructed by a successful call.

## Interpretation and Use

Efficiency tells how much metal is working: close to one for a short fin of a good conductor, falling as the fin gets longer than `1 / m`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Heat, tip temperature and efficiency of one fin.

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
  // A pin fin, 4 mm square (P = 16 mm, A = 16 mm^2) of k = 200, 10 cm long, base at 100, air at 20 with h = 25. Adiabatic tip.
  const double h = 25.0, k = 200.0, p = 0.016, a = 1.6e-5, length = 0.1;
  const auto f = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({length}), vec({100.0}), vec({20.0}));
  const double m = std::sqrt(h * p / (k * a)), big_m = std::sqrt(h * p * k * a) * 80.0;
  out.writeln("m =", m, "1/m  m L =", m * length);
  out.writeln("heat from the base", flat(f.heat), "W  closed form M tanh(m L) =", big_m * std::tanh(m * length));
  out.writeln("tip temperature", flat(f.tip), " closed form", 20.0 + 80.0 / std::cosh(m * length));
  out.writeln("efficiency", flat(f.efficiency));
}
```

Output

```text
m = 11.18034 1/m  m L = 1.11803
heat from the base [2.30944] W  closed form M tanh(m L) = 2.30944
tip temperature [67.25680]  closed form 67.25680
efficiency [0.72170]
```
### Example 2

Three tip conditions.

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
  // The three tip conditions for the same fin at two lengths. A convective tip adds the tip area; once the fin is long the three agree.
  const double h = 25.0, k = 200.0, p = 0.016, a = 1.6e-5;
  for (const double length : {0.05, 0.3}) {
    out.writeln("length", length, "m");
    for (const auto tip : {th::Tip::Adiabatic, th::Tip::Convective, th::Tip::Infinite}) {
      const auto f = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({length}), vec({100.0}), vec({20.0}), tip);
      out.writeln(tip == th::Tip::Adiabatic ? "  adiabatic " : tip == th::Tip::Convective ? "  convective" : "  infinite  ", "heat", flat(f.heat), "W  tip", flat(f.tip));
    }
  }
}
```

Output

```text
length 0.05000 m
  adiabatic  heat [1.45183] W  tip [88.94402]
  convective heat [1.47546] W  tip [88.55523]
  infinite   heat [2.86217] W  tip [20.00000]
length 0.30000 m
  adiabatic  heat [2.85519] W  tip [25.58370]
  convective heat [2.85534] W  tip [25.52211]
  infinite   heat [2.86217] W  tip [20.00000]
```
### Example 3

A batch over length.

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
  // A batch over fin length: the heat saturates and the efficiency falls as the fin gets longer.
  const double h = 25.0, k = 200.0, p = 0.016, a = 1.6e-5;
  const auto f = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({0.01, 0.03, 0.1, 0.3}), vec({100.0}), vec({20.0}));
  out.writeln("heat (W)  ", flat(f.heat));
  out.writeln("efficiency", flat(f.efficiency));
  const auto inf = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({1.0}), vec({100.0}), vec({20.0}), th::Tip::Infinite);
  out.writeln("limit for an infinitely long fin", flat(inf.heat), "W");
}
```

Output

```text
heat (W)   [0.31867, 0.92555, 2.30944, 2.85519]
efficiency [0.99585, 0.96411, 0.72170, 0.29742]
limit for an infinitely long fin [2.86217] W
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Tip.html">
    <span class="label">Previous</span>
    <span class="title">Tip</span>
  </a>
  <a class="next-link" href="fin.html">
    <span class="label">Next</span>
    <span class="title">fin</span>
  </a>
</div>
<!-- apiref:nav:end -->
