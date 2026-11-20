<!-- generated from growth_laws.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="allee_and_theta.html">
    <span class="label">Next</span>
    <span class="title">Thresholds and shape</span>
  </a>
</div>
<!-- apiref:nav:end -->
