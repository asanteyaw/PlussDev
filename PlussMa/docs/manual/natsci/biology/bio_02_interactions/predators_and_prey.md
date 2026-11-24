# Predators and prey

[Back to Biology 2: Interacting Species](../bio_02_interactions.md)

Prey grow when predators are few; predators grow when prey are plenty; each undoes the other and the two populations cycle. This page evaluates the classical predator-prey model, finds its equilibrium, and uses the quantity the model conserves to check a long integration.

## How it works

`predator_prey_rate(s, alpha, beta, gamma, delta)` takes the state `(prey, predators)` and returns `dprey = alpha prey - beta prey predators` and `dpredators = delta prey predators - gamma predators`: prey grow at `alpha` and are eaten at `beta` per predator, predators die at `gamma` and gain `delta` per prey eaten. `predator_prey_equilibrium` returns the point `(gamma/delta, alpha/beta)` where both rates are zero. `predator_prey_invariant` returns a function of the state that the exact solution never changes, so how far it drifts under a numerical integration measures the error of that integration.

## Equilibrium and a conserved quantity

The program prints the equilibrium and the rate there, then integrates from a state of `4` prey and `1` predator with `dynamics::simulate` and prints the state and the invariant at three times.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace inter = biology::population::interaction;
namespace dynamics = biology::dynamics;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // prey grows at 1.0, is eaten at 0.5 per predator; predators die at 0.75 and gain 0.25 per prey eaten
  const double alpha = 1.0, beta = 0.5, gamma = 0.75, delta = 0.25;
  const auto start = vec({4.0, 1.0});
  const auto equilibrium = inter::predator_prey_equilibrium(alpha, beta, gamma, delta, start.select(-1, 0));
  out.writeln("equilibrium (prey, predators) = ", flat(equilibrium));
  out.writeln("rate at the equilibrium       = ", flat(inter::predator_prey_rate(equilibrium, alpha, beta, gamma, delta)));

  const auto rate = [&](const pluss::Series &s) { return inter::predator_prey_rate(s, alpha, beta, gamma, delta); };
  out.writeln("invariant at t = 0            = ", flat(inter::predator_prey_invariant(start, alpha, beta, gamma, delta)));
  for (double t : {5.0, 10.0, 20.0}) {
    const auto s = dynamics::simulate(rate, start, 0.0, t, 4000);
    out.writeln("t = ", t, ": state ", flat(s), ", invariant ", flat(inter::predator_prey_invariant(s, alpha, beta, gamma, delta)));
  }
}
```

Output

```text
equilibrium (prey, predators) = [3.0000, 2.0000]
rate at the equilibrium       = [0.0000, 0.0000]
invariant at t = 0            = [0.4603]
t = 5.0000: state [1.2857, 1.6222], invariant [0.4603]
t = 10.0000: state [3.5387, 3.6195], invariant [0.4603]
t = 20.0000: state [1.2533, 1.7376], invariant [0.4603]
```

The equilibrium is `3` prey and `2` predators and both rates vanish there. Away from it the populations cycle: starting from `(4, 1)` the prey have fallen to `1.2857` by time `5` and recovered to `3.5387` by time `10`. The invariant stays at `0.4603` in all four lines, so the integration follows the exact orbit to the printed digits.

<!-- addex:start -->
## How much a predator eats

In the classical model a predator's catch grows without limit with prey density. Real predators saturate: they need time to handle each prey. The three Holling functional responses describe this, `holling1(prey, a)` linear, `holling2(prey, a, h)` saturating with handling time {math}`h`, and `holling3(prey, a, h, n)` sigmoidal. The program evaluates each at seven prey densities.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace population = biology::population;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace inter = population::interaction;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // how many prey one predator eats per unit time at prey densities from 0.5 to 40: the three Holling responses
  const auto prey = vec({0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 40.0});
  out.writeln("prey density        ", flat(prey));
  out.writeln("Holling I   (a = 0.5)            ", flat(inter::holling1(prey, 0.5)));
  out.writeln("Holling II  (a = 0.5, h = 1)     ", flat(inter::holling2(prey, 0.5, 1.0)));
  out.writeln("Holling III (a = 0.1, h = 1, n=2)", flat(inter::holling3(prey, 0.1, 1.0)));
}
```

Output

```text
prey density        [0.5000, 1.0000, 2.0000, 5.0000, 10.0000, 20.0000, 40.0000]
Holling I   (a = 0.5)            [0.2500, 0.5000, 1.0000, 2.5000, 5.0000, 10.0000, 20.0000]
Holling II  (a = 0.5, h = 1)     [0.2000, 0.3333, 0.5000, 0.7143, 0.8333, 0.9091, 0.9524]
Holling III (a = 0.1, h = 1, n=2)[0.0244, 0.0909, 0.2857, 0.7143, 0.9091, 0.9756, 0.9938]
```

The linear response keeps growing, `20.0000` at density `40`. The Holling II response saturates at the level {math}`1/h = 1`, reaching `0.9524` at `40`. The Holling III response is almost silent at low density (`0.0244` at `0.5`, against `0.2000` for type II), which is the signature of a predator that ignores rare prey, and then saturates just as quickly.

## Saturating predation

With a saturating response the prey have a refuge at low density and the community can settle to a steady coexistence. `rosenzweig_macarthur_rate(s, r, K, a, h, e, m)` combines logistic prey with a Holling II predator. The program starts two communities, one with few predators and one with many, with {math}`K = 1.5`, and follows them to time {math}`60`; the predicted coexistence point is {math}`x^* = m/(a(e - mh)) = 0.5`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace population = biology::population;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace inter = population::interaction;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // Rosenzweig-MacArthur: logistic prey (r = 1, K = 1.5) eaten with a Holling II response (a = 1, h = 0.5), converted at 0.5, predators die at 0.2
  const auto rate = [](const pluss::Series &s) { return inter::rosenzweig_macarthur_rate(s, 1.0, 1.5, 1.0, 0.5, 0.5, 0.2); };
  // the same community started with few and with many predators
  const auto start = vec({1.5, 0.1, 1.5, 2.0}).reshape({2, 2});
  for (double t : {5.0, 20.0, 60.0}) out.writeln("time ", t, "  (prey, predators) per start = ", flat(dynamics::simulate(rate, start, 0.0, t, 6000)));
  // the coexistence point where both rates vanish: handling time 0.5 gives prey x* = m / (a (e - m h)) = 0.2 / (0.5 - 0.1)
  out.writeln("coexistence point: prey x* = ", 0.2 / (1.0 * (0.5 - 0.2 * 0.5)), ", predators y* = ", 1.0 * (1.0 - 0.5 / 1.5) * (1.0 + 1.0 * 0.5 * 0.5) / 1.0);
}
```

Output

```text
time 5.000  (prey, predators) per start = [1.291, 0.285, 0.025, 1.244]
time 20.000  (prey, predators) per start = [0.408, 0.885, 0.971, 0.606]
time 60.000  (prey, predators) per start = [0.498, 0.835, 0.504, 0.830]
coexistence point: prey x* = 0.500, predators y* = 0.833
```

The two communities start apart (`1.291` prey and `0.285` predators against `0.025` and `1.244` at time `5`), swing about the coexistence point, and both end at it: `0.498` and `0.835` against the predicted `0.500` and `0.833`. Both starts reach the same point, which is the sign of a stable steady state.

## A batch of ecosystems

The rate, the equilibrium and the invariant all take arrays of parameters, so a family of ecosystems is one call. The program varies the predator death rate {math}`\gamma` over four values with the same prey and predator starting point and prints the equilibria, the state at time {math}`15` and the conserved quantity before and after.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace population = biology::population;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace inter = population::interaction;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a batch of four ecosystems that differ in the predator death rate gamma, started at the same point: the equilibria and the invariants
  const auto gamma = vec({0.5, 0.75, 1.0, 1.5});
  const auto like = gamma;
  const auto eq = inter::predator_prey_equilibrium(1.0, 0.5, gamma, 0.25, like);
  out.writeln("predator death rate         ", flat(gamma));
  out.writeln("equilibrium (prey, predator)  ", flat(eq));
  const auto start = vec({4.0, 1.0, 4.0, 1.0, 4.0, 1.0, 4.0, 1.0}).reshape({4, 2});
  const auto rate = [&](const pluss::Series &s) { return inter::predator_prey_rate(s, 1.0, 0.5, gamma, 0.25); };
  const auto later = dynamics::simulate(rate, start, 0.0, 15.0, 3000);
  out.writeln("state at time 15            ", flat(later));
  out.writeln("invariant at time 0         ", flat(inter::predator_prey_invariant(start, 1.0, 0.5, gamma, 0.25)));
  out.writeln("invariant at time 15        ", flat(inter::predator_prey_invariant(later, 1.0, 0.5, gamma, 0.25)));
}
```

Output

```text
predator death rate         [0.5000, 0.7500, 1.0000, 1.5000]
equilibrium (prey, predator)  [2.0000, 2.0000, 3.0000, 2.0000, 4.0000, 2.0000, 6.0000, 2.0000]
state at time 15            [0.4744, 1.7502, 3.6895, 0.9671, 6.5233, 2.7116, 2.9543, 2.0170]
invariant at time 0         [0.8069, 0.4603, 0.1137, -0.5794]
invariant at time 15        [0.8069, 0.4603, 0.1137, -0.5794]
```

The equilibrium prey density {math}`\gamma/\delta` grows with the predator death rate, `2.0000`, `3.0000`, `4.0000` and `6.0000`, while the predator density {math}`\alpha/\beta` stays at `2.0000`. The invariant of each ecosystem is the same at time `0` and time `15` to four digits (`0.8069`, `0.4603`, `0.1137`, `-0.5794`), so the four integrations follow their own conserved orbits.

<!-- addex:end -->

## Decisions

- Use `predator_prey_invariant` to check any long run of the classical model: it should not move.
- Use `rosenzweig_macarthur_rate` when prey have a refuge or predators saturate; the classical model's neutral cycles are not robust.
- Use `holling2` or `holling3` as soon as prey density varies by an order of magnitude: the linear response is only valid at low density.

## Pitfalls

The conserved quantity is exact for the model but not for a numerical solution of it. The program integrates the page's predator-prey system to time `20` with `20`, `100` and `4000` steps and prints the invariant each time.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace inter = biology::population::interaction;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the invariant measures the integration: a coarse step drifts off the exact orbit
  const double alpha = 1.0, beta = 0.5, gamma = 0.75, delta = 0.25;
  const auto start = vec({4.0, 1.0});
  const auto rate = [&](const pluss::Series &s) { return inter::predator_prey_rate(s, alpha, beta, gamma, delta); };
  out.writeln("invariant at t = 0 = ", flat(inter::predator_prey_invariant(start, alpha, beta, gamma, delta)));
  for (int steps : {20, 100, 4000}) {
    const auto s = dynamics::simulate(rate, start, 0.0, 20.0, steps);
    out.writeln(steps, " steps: invariant at t = 20 = ", flat(inter::predator_prey_invariant(s, alpha, beta, gamma, delta)));
  }
}
```

Output

```text
invariant at t = 0 = [0.4603]
20 steps: invariant at t = 20 = [0.4412]
100 steps: invariant at t = 20 = [0.4603]
4000 steps: invariant at t = 20 = [0.4603]
```

The invariant starts at `0.4603`. With `20` steps it has drifted to `0.4412`, so the run has left the true orbit; with `100` and `4000` steps it stays at `0.4603`. A drifting invariant is the warning that the step count is too small, and the check costs one call.

## Where next

[Competing species](competition.md) gives the other classic interaction, and [Mass-action networks](../bio_25_networks/mass_action_networks.md) writes such models as reactions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="competition.html">
    <span class="label">Next</span>
    <span class="title">Competing species</span>
  </a>
</div>
<!-- apiref:nav:end -->
