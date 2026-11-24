# Thresholds and shape

[Back to Biology 1: Growth](../bio_01_growth.md)

Logistic growth assumes a population is always better off with more company up to the capacity. Two variations break that: a strong Allee effect makes a small population shrink, so growth has a threshold, and the theta-logistic law changes how sharply growth slows near capacity. This page evaluates both rates, shows the threshold deciding between extinction and recovery, and compares the shape parameter.

## How it works

`strong_allee_rate(n, r, threshold, k)` returns `r n (n/A - 1)(1 - n/k)` with `0 < A < k`. It is zero at `0`, at the threshold `A` and at the capacity `k`; between `A` and `k` it is positive, below `A` it is negative. So `A` is an unstable equilibrium that separates populations that die out from populations that grow to `k`. `theta_logistic_rate(n, r, k, theta)` is `r n (1 - (n/k)^theta)`; `theta = 1` is the ordinary logistic law, a larger `theta` keeps the rate high until the population is closer to `k`. Both functions take arrays for every argument, and the integrator `dynamics::simulate` advances the whole batch together.

## The threshold decides the outcome

The program prints the Allee rate at four sizes, then integrates two populations that start on either side of the threshold `A = 2` to time `10`, and finally prints the theta-logistic rate at the same size for three values of `theta` in one call.

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
  // strong Allee effect: r = 1, threshold A = 2, capacity K = 10
  const auto sizes = vec({1.0, 2.0, 5.0, 10.0});
  out.writeln("size          ", flat(sizes));
  out.writeln("Allee rate    ", flat(growth::strong_allee_rate(sizes, 1.0, 2.0, 10.0)));
  // start just below and just above the threshold, integrate to t = 10
  const auto start = vec({1.9, 2.1});
  const auto rate = [](const pluss::Series &n) { return growth::strong_allee_rate(n, 1.0, 2.0, 10.0); };
  out.writeln("start         ", flat(start));
  out.writeln("size at t=10  ", flat(dynamics::simulate(rate, start, 0.0, 10.0, 400)));
  // theta-logistic: theta below 1 slows growth near zero, above 1 delays the slowdown
  const auto n = vec({5.0, 5.0, 5.0});
  const auto theta = vec({0.5, 1.0, 2.0});
  out.writeln("theta         ", flat(theta));
  out.writeln("rate at n=5   ", flat(growth::theta_logistic_rate(n, 0.7, 10.0, theta)));
}
```

Output

```text
size          [1.0000, 2.0000, 5.0000, 10.0000]
Allee rate    [-0.4500, 0.0000, 3.7500, 0.0000]
start         [1.9000, 2.1000]
size at t=10  [0.0035, 10.0000]
theta         [0.5000, 1.0000, 2.0000]
rate at n=5   [1.0251, 1.7500, 2.6250]
```

At sizes `1`, `2`, `5` and `10` the rate is negative, zero, positive and zero: the population below the threshold declines, the threshold itself is the balance point, and the capacity is where growth stops. The two populations that start at `1.9` and `2.1` end at `0.0035` and `10.0000`; a difference of `0.2` in the starting size decides between extinction and the capacity. For the theta-logistic law, the rate at `n = 5` grows from `1.0251` to `1.7500` to `2.6250` as `theta` goes from `0.5` to `1` to `2`.

<!-- addex:start -->
## Which side of the threshold

The threshold can be watched in a batch. The program starts five populations at `1.0`, `1.9`, `2.1`, `4.0` and `12.0` with threshold `2` and capacity `10` and integrates them together, printing the sizes at three times.

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
  // a strong Allee effect with threshold 2 and capacity 10: a population on either side of 2, in one batch
  const auto start = vec({1.0, 1.9, 2.1, 4.0, 12.0});
  const auto rate = [](const pluss::Series &n) { return growth::strong_allee_rate(n, 1.0, 2.0, 10.0); };
  out.writeln("start      ", flat(start));
  for (double t : {2.0, 5.0, 10.0}) out.writeln("time ", t, "  ", flat(dynamics::simulate(rate, start, 0.0, t, 800)));
}
```

Output

```text
start      [1.0000, 1.9000, 2.1000, 4.0000, 12.0000]
time 2.0000  [0.2644, 1.5712, 2.5761, 9.9479, 10.0005]
time 5.0000  [0.0155, 0.3943, 9.8986, 10.0000, 10.0000]
time 10.0000  [0.0001, 0.0035, 10.0000, 10.0000, 10.0000]
```

The two populations below the threshold, `1.0` and `1.9`, decline toward zero (`0.0001` and `0.0035` at time `10`); the three above it end at the capacity `10.0000`. The one that starts above the capacity, at `12`, falls back to it, and the one at `2.1`, only just above the threshold, takes longer to recover (`2.5761` at time `2`, `9.8986` at time `5`) than the one at `4`.

## Births and deaths, and the shape parameter

The Allee effect can also be written with explicit births {math}`b_0 n/(b_1 + n)` and deaths {math}`d + cn`, which gives {math}`\mathrm{d}n/\mathrm{d}t = (b_0 n/(b_1 + n) - d - cn)\,n`. `allee_birth_death_rate(n, b0, b1, d, c)` evaluates it. The program prints that rate at eight sizes for {math}`b_0 = 4`, {math}`b_1 = 2`, {math}`d = 0.5`, {math}`c = 0.15`, then the theta-logistic rate at half the capacity for four values of {math}`\theta`.

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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the birth-death form of the Allee effect: births b0 n / (b1 + n), deaths d + c n; the rate is negative when the population is too sparse and again when it is overcrowded
  const auto n = vec({0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 40.0});
  out.writeln("size                  ", flat(n));
  out.writeln("rate dn/dt             ", flat(growth::allee_birth_death_rate(n, 4.0, 2.0, 0.5, 0.15)));
  // the theta-logistic rate for a population at half capacity as theta increases
  const auto half = vec({5.0, 5.0, 5.0, 5.0});
  out.writeln("theta                 ", flat(vec({0.25, 0.5, 1.0, 4.0})));
  out.writeln("theta-logistic rate   ", flat(growth::theta_logistic_rate(half, 0.7, 10.0, vec({0.25, 0.5, 1.0, 4.0}))));
}
```

Output

```text
size                  [0.2000, 0.5000, 1.0000, 2.0000, 5.0000, 10.0000, 20.0000, 40.0000]
rate dn/dt             [-0.0333, 0.1125, 0.6833, 2.4000, 8.0357, 13.3333, 2.7273, -107.6190]
theta                 [0.2500, 0.5000, 1.0000, 4.0000]
theta-logistic rate   [0.5569, 1.0251, 1.7500, 3.2812]
```

The rate is negative at the smallest size `0.2` (`-0.0333`), when births are too few to replace deaths, positive from `0.5` to `20`, and negative again at `40` (`-107.6190`), when crowding dominates. For the theta-logistic law the rate at half the capacity rises with {math}`\theta`, from `0.5569` at {math}`\theta = 0.25` to `3.2812` at {math}`\theta = 4`, because a larger {math}`\theta` keeps growth near its maximum until the population is closer to the capacity.

## Arguments that are checked

The Allee threshold must lie below the carrying capacity, and the shape parameter must be positive. The program passes a threshold of `12` with capacity `10`, and a {math}`\theta` of zero.

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
  // an Allee threshold has to lie below the capacity
  try {
    (void)growth::strong_allee_rate(vec({3.0}), 1.0, 12.0, 10.0);
  } catch (const pluss::Error &e) {
    out.writeln("threshold above capacity: ", e.message());
  }
  try {
    (void)growth::theta_logistic_rate(vec({3.0}), 0.7, 10.0, 0.0);
  } catch (const pluss::Error &e) {
    out.writeln("theta of zero:            ", e.message());
  }
}
```

Output

```text
threshold above capacity:  natsci: allee: threshold must be below carrying capacity
theta of zero:             natsci: theta_logistic: theta must be finite and strictly positive
```

Both calls raise a `pluss::Error` that names the function and the rule: a threshold must be below the capacity, and {math}`\theta` must be finite and strictly positive.

<!-- addex:end -->

## Decisions

- Use `strong_allee_rate` when small populations are known to fail (mate finding, group defence); use `theta_logistic_rate` when only the sharpness of density limitation is in question.
- Integrate a batch of starting sizes together, as in the examples, to find the threshold by bracketing instead of by solving.

## Pitfalls

The threshold is an unstable equilibrium, so a population started exactly on it is in balance but not safe. The program starts three populations at `2`, at `2 + 10^-6` and at `2 - 10^-6`, with the threshold at `2` and the capacity at `10`, and prints them at three times.

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
  // a population exactly at the Allee threshold is in balance, but the balance is unstable
  const auto rate = [](const pluss::Series &n) { return growth::strong_allee_rate(n, 0.5, 2.0, 10.0); };
  const auto start = vec({2.0, 2.000001, 1.999999});
  for (double t : {10.0, 30.0, 50.0}) out.writeln("time ", t, ": ", flat(dynamics::simulate(rate, start, 0.0, t, 5000)));
}
```

Output

```text
time 10.0000: [2.0000, 2.0001, 1.9999]
time 30.0000: [2.0000, 2.1732, 1.8467]
time 50.0000: [2.0000, 10.0000, 0.0020]
```

The first stays at `2.0000` because its rate is exactly zero. The other two, a millionth apart, are still at `2.0001` and `1.9999` at time `10`, but they have separated by time `30` (`2.1732` and `1.8467`) and by time `50` one is at the capacity (`10.0000`) and the other has all but vanished (`0.0020`). A tiny measurement error in the starting size is therefore enough to reverse the outcome near the threshold; do not trust a prediction of the fate of a population whose size is within the measurement error of {math}`A`.

## Where next

[Growth laws](growth_laws.md) gives the plain logistic law, and [Competing species](../bio_02_interactions/competition.md) puts several populations with limits on one another into one model.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="growth_laws.html">
    <span class="label">Previous</span>
    <span class="title">Growth laws</span>
  </a>
  <a class="next-link" href="age_structure.html">
    <span class="label">Next</span>
    <span class="title">Age structure</span>
  </a>
</div>
<!-- apiref:nav:end -->
