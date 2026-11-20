<!-- generated from extinction.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Extinction

[Back to Biology 3: Demography and Extinction](../bio_03_demography.md)

A population that grows on average can still die out by chance, and a bounded population can last a long time without being safe. This page models births and deaths as random events: the expected time to extinction of a bounded population, the state it lingers in before extinction, and the chance that a growing population started from a few founders still goes extinct.

## How it works

A birth-death chain gives each population size `n` a birth rate and a death rate; `generator(birth, death)` turns the two vectors into the transition-rate matrix. `mean_time_to_extinction(q)` is the expected time to reach zero from each size `1..N`. `quasi_stationary(q)` finds the distribution a population settles into while it has not yet gone extinct and its `decay_rate`, the constant rate at which it then goes extinct. For an unbounded population with per-capita rates `birth` and `death`, `linear_ultimate_extinction` is `(death/birth)^n` from `n` founders and `linear_extinction_probability` the chance that extinction has happened by time `t`.

## Time to extinction

The first part of the program bounds a population at `20` animals with births at `0.6` per animal and deaths at `0.5` and prints the expected time to extinction and the quasi-stationary state. The second part removes the bound and prints the extinction probability for one and for five founders, and by three times for five founders.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace bd = biology::population::birth_death;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a population of at most 20 animals. Each breeds at rate 0.6 while there is room; each dies at rate 0.5.
  const int capacity = 20;
  std::vector<double> births, deaths;
  for (int n = 0; n <= capacity; ++n) {
    births.push_back(n == capacity ? 0.0 : 0.6 * n);
    deaths.push_back(0.5 * n);
  }
  const auto q = bd::generator(vec(births), vec(deaths));
  const auto tau = flat(bd::mean_time_to_extinction(q));
  out.writeln("expected time to extinction from 1, 5, 10, 20 animals: ", std::vector<double>{tau[0], tau[4], tau[9], tau[19]});
  const auto qsd = bd::quasi_stationary(q);
  out.writeln("quasi-stationary extinction rate = ", flat(qsd.decay_rate));
  const auto mode = flat(qsd.distribution);
  out.writeln("probability of 1, 10, 20 animals in the quasi-stationary state: ", std::vector<double>{mode[0], mode[9], mode[19]});

  // the unbounded linear process: extinction chance for a founder group of 1 and of 5
  out.writeln("ultimate extinction, birth 0.6 death 0.5, from 1 and from 5 = ",
              std::vector<double>{bd::linear_ultimate_extinction(1, 0.6, 0.5), bd::linear_ultimate_extinction(5, 0.6, 0.5)});
  // extinction chance by time t for 5 founders
  out.writeln("extinct by t = 5, 20, 100: ", flat(bd::linear_extinction_probability(vec({5.0, 5.0, 5.0}), 0.6, 0.5, vec({5.0, 20.0, 100.0}))));
}
```

Output

```text
expected time to extinction from 1, 5, 10, 20 animals: [29.9588, 98.7952, 131.5289, 143.7757]
quasi-stationary extinction rate = [0.0075]
probability of 1, 10, 20 animals in the quasi-stationary state: [0.0151, 0.0369, 0.1258]
ultimate extinction, birth 0.6 death 0.5, from 1 and from 5 = [0.8333, 0.4019]
extinct by t = 5, 20, 100: [0.1281, 0.3533, 0.4019]
```

Births exceed deaths, yet a single founder in a population limited to `20` animals goes extinct after `29.9588` time units on average, while a population of `20` lasts `143.7757`. Before extinction the population lingers in a quasi-stationary state in which the chance of going extinct is `0.0075` per unit time. Without the bound, a single founder leaves no descendants with probability `0.8333` (`0.5/0.6`) and five founders with probability `0.4019`; the extinction by time `100` for five founders is `0.4019`, matching the limit, and most of it has happened by time `20` (`0.3533`).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stage_matrices.html">
    <span class="label">Previous</span>
    <span class="title">Stage matrices</span>
  </a>
</div>
<!-- apiref:nav:end -->
