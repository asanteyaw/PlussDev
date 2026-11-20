<!-- generated from allee_and_theta.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

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
