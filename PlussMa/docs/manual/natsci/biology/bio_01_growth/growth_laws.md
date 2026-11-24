# Growth laws

[Back to Biology 1: Growth](../bio_01_growth.md)

A population of `n` individuals with growth rate `r` and room for `k` of them grows quickly while it is small and slows as it fills the space. This page takes the logistic and Gompertz laws, which both level off at the carrying capacity `k`, compares the exact solution of the logistic law with a numerical integration of its rate, and then solves three growth rates in a single call.

## How it works

Each law comes in two forms. `logistic_rate(n, r, k)` returns the rate of change `dn/dt = r n (1 - n/k)` at the current size, which is what an integrator needs. `logistic(n0, r, k, t)` returns the size reached at time `t` from `n0`, which has a closed form: `k / (1 + ((k - n0)/n0) e^{-rt})`. Gompertz growth has the same two forms with the rate `r n ln(k/n)`. The rate and the closed form describe the same curve, so one checks the other.

Every argument may be an array. The rate `r`, the capacity `k` and the time `t` are broadcast against `n0`, so a batch of populations, or a batch of growth rates, is one call.

## The exact solution against the integrator

The program starts a population of `1.5` with `r = 0.7` and `k = 10`, takes its logistic size at time `2` in closed form, and integrates the rate with `dynamics::simulate` over 200 steps. It then prints the Gompertz size for the same starting point, and the logistic size for three growth rates in one call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace growth = biology::population::growth;
namespace dynamics = biology::dynamics;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto n0 = vec({1.5});
  const auto exact = growth::logistic(n0, 0.7, 10.0, 2.0);
  const auto rate = [](const pluss::Series &n) { return growth::logistic_rate(n, 0.7, 10.0); };
  const auto numeric = dynamics::simulate(rate, n0, 0.0, 2.0, 200);
  out.writeln("closed form n(2) = ", flat(exact));
  out.writeln("RK4 n(2)         = ", flat(numeric));
  out.writeln("Gompertz n(2)    = ", flat(growth::gompertz(n0, 0.7, 10.0, 2.0)));
  const auto lanes = vec({1.5, 1.5, 1.5});
  out.writeln("three growth rates, one call: ", flat(growth::logistic(lanes, vec({0.1, 0.7, 1.0}), 10.0, 2.0)));
}
```

Output

```text
closed form n(2) = [4.1712]
RK4 n(2)         = [4.1712]
Gompertz n(2)    = [6.2636]
three growth rates, one call: [1.7732, 4.1712, 5.6596]
```

The closed form and the integration agree to the four digits printed, and Gompertz growth, which fills the space faster at the start, is further along at the same time. In the last line the slowest rate has barely left the starting size while the fastest has passed half the capacity.

<!-- addex:start -->
## Early growth and the rate functions

At small sizes the population has all the room it needs, so the logistic law is close to the exponential law {math}`n(t) = n_0 e^{rt}`. The program starts four copies of a population of {math}`0.5` with growth rate {math}`0.4` and capacity {math}`100`, evaluates both laws at four times in one call each, and prints the two rate functions at the starting size.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace growth = biology::population::growth;
namespace dynamics = biology::dynamics;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // early growth is exponential: with n0 = 0.5 far below the capacity 100, the logistic and exponential curves agree at first
  const auto n0 = vec({0.5, 0.5, 0.5, 0.5});
  const auto t = vec({0.0, 1.0, 3.0, 6.0});
  out.writeln("time                ", flat(t));
  out.writeln("exponential growth  ", flat(growth::exponential(n0, 0.4, t)));
  out.writeln("logistic growth     ", flat(growth::logistic(n0, 0.4, 100.0, t)));
  // the rate functions are what an integrator needs: the logistic rate at the same sizes
  out.writeln("logistic rate at n0 ", flat(growth::logistic_rate(n0, 0.4, 100.0)));
  out.writeln("exponential rate    ", flat(growth::exponential_rate(n0, 0.4)));
}
```

Output

```text
time                [0.0000, 1.0000, 3.0000, 6.0000]
exponential growth  [0.5000, 0.7459, 1.6601, 5.5116]
logistic growth     [0.5000, 0.7441, 1.6410, 5.2486]
logistic rate at n0 [0.1990, 0.1990, 0.1990, 0.1990]
exponential rate    [0.2000, 0.2000, 0.2000, 0.2000]
```

At time `1` the two curves are `0.7459` and `0.7441`, and they separate slowly: by time `6` the exponential has reached `5.5116` and the logistic `5.2486`, because the logistic law has begun to feel the capacity. The rates show the same thing at the start: `0.2000` for exponential growth and `0.1990` for logistic growth, which is smaller by the factor {math}`1 - n/k = 0.995`.

## Arguments that are checked

The laws are defined only for positive sizes and capacities, and the functions say so. The program passes a zero capacity and a negative starting size to `logistic`, and a zero size to the Gompertz rate, which contains {math}`\ln(k/n)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace growth = biology::population::growth;

int main() {
  OutputEngine out;
  // growth laws check their arguments: a carrying capacity must be positive, a starting size too
  try {
    (void)growth::logistic(vec({1.5}), 0.7, 0.0, 2.0);
  } catch (const pluss::Error &e) {
    out.writeln("zero capacity:      ", e.message());
  }
  try {
    (void)growth::logistic(vec({-1.0}), 0.7, 10.0, 2.0);
  } catch (const pluss::Error &e) {
    out.writeln("negative start:     ", e.message());
  }
  try {
    (void)growth::gompertz_rate(vec({0.0}), 0.7, 10.0);
  } catch (const pluss::Error &e) {
    out.writeln("Gompertz at n = 0:  ", e.message());
  }
}
```

Output

```text
zero capacity:       natsci: logistic: carrying capacity must be finite and strictly positive
negative start:      natsci: logistic: n0 must be strictly positive
Gompertz at n = 0:   natsci: gompertz: n must be strictly positive
```

Each call raises a `pluss::Error` whose message names the function and the argument, and nothing is computed from a value the law cannot accept. The three messages are the ones printed above, and `e.message()` is the text a program can show to its user.

<!-- addex:end -->

## Decisions

- Integrate `logistic_rate` only when the rate itself changes (a harvest, a seasonal `r`); when only the size at a time is wanted, call `logistic` and skip the integrator.
- Choose the number of steps from the fastest rate in the problem, not from the length of the run: the product {math}`r\,\Delta t` must be small.

## Pitfalls

Two mistakes are easy to make: integrating a fast law with too few steps, and mixing time units between the rate and the time. The program integrates a logistic law with {math}`r = 3` at four step counts and compares the result with the closed form, then evaluates the same curve with `r` per day and per week.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace growth = biology::population::growth;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the integrator needs enough steps: with a fast growth rate r = 3, compare 1, 2, 8 and 200 steps with the closed form at time 2
  const auto n0 = vec({1.5});
  const auto rate = [](const pluss::Series &n) { return growth::logistic_rate(n, 3.0, 10.0); };
  out.writeln("closed form      = ", flat(growth::logistic(n0, 3.0, 10.0, 2.0)));
  for (int steps : {1, 2, 8, 200}) out.writeln(steps, " steps         = ", flat(dynamics::simulate(rate, n0, 0.0, 2.0, steps)));
  // and the rate r is per unit time: the same curve in days and in weeks differs by the factor 7
  out.writeln("r = 0.1 per day, 14 days  = ", flat(growth::logistic(n0, 0.1, 10.0, 14.0)));
  out.writeln("r = 0.1 per week, 2 weeks = ", flat(growth::logistic(n0, 0.1, 10.0, 2.0)));
}
```

Output

```text
closed form      = [9.8615]
1 steps         = [11.6835]
2 steps         = [7.6117]
8 steps         = [9.8600]
200 steps         = [9.8615]
r = 0.1 per day, 14 days  = [4.1712]
r = 0.1 per week, 2 weeks = [1.7732]
```

A single step overshoots the capacity (`11.6835` against a limit of `10`) and two steps undershoot (`7.6117`); only from about `8` steps do the digits settle (`9.8600`, then `9.8615` at `200`). If a population can exceed its carrying capacity in your output, the step count is the first thing to check. The last two lines show the unit mistake: `r = 0.1` per day over `14` days and `r = 0.1` per week over `2` weeks are different problems, `4.1712` against `1.7732`; keep `r` and `t` in the same unit.

## Where next

[Thresholds and shape](allee_and_theta.md) adds the Allee effect to the growth law, and [Age structure](age_structure.md) replaces the single rate `r` with a schedule by age.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="allee_and_theta.html">
    <span class="label">Next</span>
    <span class="title">Thresholds and shape</span>
  </a>
</div>
<!-- apiref:nav:end -->
