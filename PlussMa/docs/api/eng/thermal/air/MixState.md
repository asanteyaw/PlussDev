<!-- generated from MixState.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::MixState`

The state of a mixture of two moist-air streams.

## Signature

```cpp
thermal::MixState
```

## What It Does

Holds the temperature and humidity ratio of the stream that results from mixing two streams adiabatically.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `temperature` | The mixture temperature in degrees Celsius. |  |
| `w` | The mixture humidity ratio in kg/kg. |  |

## Returns

A value returned by `mix`.

## Errors

None: a state is only constructed by a successful call.

## Interpretation and Use

The state is not checked against saturation: a mixture of two saturated streams can be supersaturated, and `rh` of the result then exceeds one.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Two streams in equal parts.

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
  // Equal mass flows of 30 degree air (w = 0.012) and 10 degree air (w = 0.006): w averages, enthalpy averages, temperature follows.
  const auto m = th::mix(vec({30.0}), vec({0.012}), vec({1.0}), vec({10.0}), vec({0.006}), vec({1.0}));
  out.writeln("mixed w", flat(m.w), " temperature", flat(m.temperature));
  out.writeln("enthalpy of the mixture", flat(th::enth(m.temperature, m.w)), " mean of the two", 0.5 * (one(th::enth(vec({30.0}), vec({0.012}))) + one(th::enth(vec({10.0}), vec({0.006})))));
}
```

Output

```text
mixed w [0.00900]  temperature [20.05456]
enthalpy of the mixture [43019.60000]  mean of the two 43019.60000
```
### Example 2

An outdoor-air fraction.

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
  // Unequal flows: an economiser mixing outdoor air (-5 degrees, w = 0.002) into return air (22 degrees, w = 0.008). Fraction of outdoor air from 0 to 1.
  const pluss::Series outdoor = vec({0.0, 0.25, 0.5, 0.75, 1.0});
  const auto m = th::mix(vec({-5.0}), vec({0.002}), outdoor.add(1e-9), vec({22.0}), vec({0.008}), outdoor.neg().add(1.0).add(1e-9));
  out.writeln("outdoor fraction", flat(outdoor));
  out.writeln("mixed temperature", flat(m.temperature));
  out.writeln("mixed w          ", flat(m.w));
}
```

Output

```text
outdoor fraction [0.00000, 0.25000, 0.50000, 0.75000, 1.00000]
mixed temperature [22.00000, 15.30549, 8.57419, 1.80580, -5.00000]
mixed w           [0.00800, 0.00650, 0.00500, 0.00350, 0.00200]
```
### Example 3

A mixture that forms fog.

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
  // Mixing two saturated streams makes supersaturated air: the straight-line mix lies above the saturation curve and fog forms.
  const double p = 101325.0;
  const pluss::Series w1 = th::ratio(vec({5.0}), vec({1.0}), vec({p})), w2 = th::ratio(vec({30.0}), vec({1.0}), vec({p}));
  const auto m = th::mix(vec({5.0}), w1, vec({1.0}), vec({30.0}), w2, vec({1.0}));
  out.writeln("mixed temperature", flat(m.temperature), " w", flat(m.w));
  out.writeln("saturation w at that temperature", flat(th::ratio(m.temperature, vec({1.0}), vec({p}))), "-> supersaturated, relative humidity", flat(th::rh(m.temperature, m.w, vec({p}))));
}
```

Output

```text
mixed temperature [17.74455]  w [0.01630]
saturation w at that temperature [0.01272] -> supersaturated, relative humidity [1.27406]
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="wet.html">
    <span class="label">Previous</span>
    <span class="title">wet</span>
  </a>
  <a class="next-link" href="mix.html">
    <span class="label">Next</span>
    <span class="title">mix</span>
  </a>
</div>
<!-- apiref:nav:end -->
