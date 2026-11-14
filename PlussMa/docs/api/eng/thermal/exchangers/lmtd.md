<!-- generated from lmtd.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::lmtd`

The log-mean temperature difference of an exchanger.

## Signature

```cpp
Series thermal::lmtd(const Series& hot_in,
                      const Series& hot_out,
                      const Series& cold_in,
                      const Series& cold_out,
                      thermal::Flow flow);
```

## What It Does

Returns `(dT1 - dT2) / ln(dT1 / dT2)` for the temperature differences at the two ends of a parallel-flow or counter-flow exchanger, so that `duty = UA * LMTD`. For counter flow the ends pair hot-in with cold-out and hot-out with cold-in; for parallel flow hot-in with cold-in and hot-out with cold-out. Equal end differences give their common value without dividing by zero.

## Parameters

`hot_in`
: The hot inlet temperature.

`hot_out`
: The hot outlet temperature.

`cold_in`
: The cold inlet temperature.

`cold_out`
: The cold outlet temperature.

`flow`
: `Parallel` or `Counter`.

## Returns

The log-mean difference with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for an input that is not a real floating array or not finite, for an arrangement other than parallel or counter flow, and when the hot stream is not warmer than the cold stream at both ends.

## Interpretation and Use

Use `lmtd` to rate an exchanger whose four temperatures are known (a measurement or a specification). For other arrangements use `rate`, which handles every arrangement through effectiveness.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Counter flow with equal and unequal ends.

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
### Example 2

Parallel against counter flow.

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
### Example 3

Duty from UA times LMTD.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rate.html">
    <span class="label">Previous</span>
    <span class="title">rate</span>
  </a>
</div>
<!-- apiref:nav:end -->
