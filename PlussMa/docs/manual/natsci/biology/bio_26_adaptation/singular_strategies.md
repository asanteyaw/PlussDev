# Singular strategies

[Back to Biology 26: Adaptive Dynamics](../bio_26_adaptation.md)

A trait such as beak size evolves when rare mutants that differ a little from the common type can invade. You want to know in which direction the trait moves, where it stops, and whether that point is a final state or a point at which the population splits into two types. This page computes the direction of selection and its end points for a population competing for a limited resource, and classifies them.

## How it works

Let {math}`f(y, x)` be the invasion fitness, the growth rate of a rare mutant with trait {math}`y` in a population of residents with trait {math}`x`. A mutant can invade when {math}`f > 0`. The trait moves in the direction of the selection gradient

```{math}
g(x) = \left.\frac{\partial f(y, x)}{\partial y}\right|_{y = x}
```

until it reaches a singular strategy {math}`x^*` with {math}`g(x^*) = 0`. Whether that point attracts nearby traits is decided by {math}`f_{xy} + f_{yy}` (convergence stability, negative), and whether it resists invasion by nearby mutants by {math}`f_{yy}` (evolutionary stability, negative). A singular strategy that attracts but can be invaded is a branching point. `community::selection_gradient(f, x)`, `community::singular_strategy(f, x0)`, `community::second_derivatives(f, x)` and `community::classify(f, x)` compute these for a function `f(mutant, resident)`. The resource has its best trait at {math}`0`, {math}`K(x) = e^{-x^2/2\sigma_k^2}`, and competition is strongest between similar individuals, {math}`\alpha(y, x) = e^{-(y-x)^2/2\sigma_a^2}`, which gives {math}`f(y, x) = 1 - \alpha(y, x)K(x)/K(y)` through `community::logistic_competition_fitness`.

## The direction of selection

The program fixes {math}`\sigma_k = 1` and {math}`\sigma_a = 1.5`, evaluates the selection gradient at five residents, and runs the search for the singular strategy from two starting traits.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the trait x is, say, beak size. Resources are richest at x = 0: carrying capacity K(x) = exp(-x^2 / 2 sk^2).
// Competition is strongest between similar individuals: alpha(y, x) = exp(-(y - x)^2 / 2 sa^2). The invasion fitness of a mutant y in a resident x is 1 - alpha K(x) / K(y).
static community::Fitness landscape(double sk, double sa) {
  return [=](const pluss::Series &mutant, const pluss::Series &resident) {
    const auto carrying = [=](const pluss::Series &x) { return x.square().mul(-0.5 / (sk * sk)).exp(); };
    const auto kernel = [=](const pluss::Series &y, const pluss::Series &x) { return y.sub(x).square().mul(-0.5 / (sa * sa)).exp(); };
    return community::logistic_competition_fitness(mutant, resident, carrying, kernel);
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // the direction of selection: the selection gradient is the slope of the invasion fitness at a resident. Residents on both sides of the optimum x = 0, with sk = 1 and sa = 1.5
  const auto fitness = landscape(1.0, 1.5);
  const auto x = vec({-1.5, -0.5, 0.0, 0.5, 1.5});
  out.writeln("resident trait x            ", flat(x));
  out.writeln("selection gradient          ", flat(community::selection_gradient(fitness, x)));
  // evolution moves the trait up the gradient until it vanishes: the singular strategy
  out.writeln("singular strategy from x = 1.0  ", flat(community::singular_strategy(fitness, vec({1.0}))));
  out.writeln("distance of the singular strategy from x = -2.0 to 0 = ", std::abs(flat(community::singular_strategy(fitness, vec({-2.0})))[0]));
}
```

Output

```text
resident trait x            [-1.50000, -0.50000, 0.00000, 0.50000, 1.50000]
selection gradient          [1.50000, 0.50000, 0.00000, -0.50000, -1.50000]
singular strategy from x = 1.0  [0.00000]
distance of the singular strategy from x = -2.0 to 0 = 0.00000
```

The gradient is `1.50000`, `0.50000`, `0.00000`, `-0.50000` and `-1.50000` at the residents `-1.5`, `-0.5`, `0`, `0.5` and `1.5`: it equals {math}`-x/\sigma_k^2`, so selection always pushes the trait toward the resource optimum at {math}`0` from either side. The search finds the singular strategy `0.00000` from both starting points.

## Is the end point stable?

The singular strategy is at {math}`0` for every competition width. The program classifies it for five values of {math}`\sigma_a`, with {math}`\sigma_k = 1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the trait x is, say, beak size. Resources are richest at x = 0: carrying capacity K(x) = exp(-x^2 / 2 sk^2).
// Competition is strongest between similar individuals: alpha(y, x) = exp(-(y - x)^2 / 2 sa^2). The invasion fitness of a mutant y in a resident x is 1 - alpha K(x) / K(y).
static community::Fitness landscape(double sk, double sa) {
  return [=](const pluss::Series &mutant, const pluss::Series &resident) {
    const auto carrying = [=](const pluss::Series &x) { return x.square().mul(-0.5 / (sk * sk)).exp(); };
    const auto kernel = [=](const pluss::Series &y, const pluss::Series &x) { return y.sub(x).square().mul(-0.5 / (sa * sa)).exp(); };
    return community::logistic_competition_fitness(mutant, resident, carrying, kernel);
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto x_star = vec({0.0});
  // is the singular strategy where evolution stops, or where the population splits in two? Competition broader than the resource (sa > sk) leaves it stable; narrower competition makes it unstable
  for (double sa : {0.5, 0.8, 1.0, 1.2, 2.0}) {
    const auto fitness = landscape(1.0, sa);
    const auto c = community::classify(fitness, x_star);
    const auto yes = [](const pluss::Series &b) { return flat(b.to(pluss::ScalarType::Float64))[0] > 0.5 ? "yes" : "no"; };
    out.writeln("competition width ", sa, ": convergence stable ", yes(c.convergence_stable), ", evolutionarily stable ", yes(c.evolutionarily_stable), ", branching point ", yes(c.branching), ", end point of evolution ", yes(c.css));
  }
}
```

Output

```text
competition width 0.5000: convergence stable yes, evolutionarily stable no, branching point yes, end point of evolution no
competition width 0.8000: convergence stable yes, evolutionarily stable no, branching point yes, end point of evolution no
competition width 1.0000: convergence stable yes, evolutionarily stable no, branching point no, end point of evolution no
competition width 1.2000: convergence stable yes, evolutionarily stable yes, branching point no, end point of evolution yes
competition width 2.0000: convergence stable yes, evolutionarily stable yes, branching point no, end point of evolution yes
```

The classification depends on whether competition is broader or narrower than the resource. For narrow competition (`0.5` and `0.8`) the point attracts the trait but can be invaded: it is a branching point, where the population splits into two types. For broad competition (`1.2` and `2.0`) it is both attracting and uninvadable, the end of evolution. At {math}`\sigma_a = 1`, equal to {math}`\sigma_k`, the curvature vanishes and the classification is the borderline case.

## The second derivatives behind the classification

The program prints the curvature in the mutant trait {math}`f_{yy}` and the mixed derivative {math}`f_{xy}` for three competition widths, with their sum.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the trait x is, say, beak size. Resources are richest at x = 0: carrying capacity K(x) = exp(-x^2 / 2 sk^2).
// Competition is strongest between similar individuals: alpha(y, x) = exp(-(y - x)^2 / 2 sa^2). The invasion fitness of a mutant y in a resident x is 1 - alpha K(x) / K(y).
static community::Fitness landscape(double sk, double sa) {
  return [=](const pluss::Series &mutant, const pluss::Series &resident) {
    const auto carrying = [=](const pluss::Series &x) { return x.square().mul(-0.5 / (sk * sk)).exp(); };
    const auto kernel = [=](const pluss::Series &y, const pluss::Series &x) { return y.sub(x).square().mul(-0.5 / (sa * sa)).exp(); };
    return community::logistic_competition_fitness(mutant, resident, carrying, kernel);
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the second derivatives that classify the singular strategy, as a function of the competition width
  const auto x_star = vec({0.0});
  out.writeln("sa      f_yy (curvature in the mutant)   f_xy (mixed)    f_xy + f_yy");
  for (double sa : {0.5, 1.0, 1.5}) {
    const auto d = community::second_derivatives(landscape(1.0, sa), x_star);
    out.writeln(sa, "   ", flat(d.fyy)[0], "   ", flat(d.fxy)[0], "   ", flat(d.fxy)[0] + flat(d.fyy)[0]);
  }
}
```

Output

```text
sa      f_yy (curvature in the mutant)   f_xy (mixed)    f_xy + f_yy
0.5000   3.0000   -4.0000   -1.0000
1.0000   0.0000   -1.0000   -1.0000
1.5000   -0.5556   -0.4444   -1.0000
```

The sum {math}`f_{xy} + f_{yy}` is `-1.0000` for all three, so the singular strategy always attracts. The sign of {math}`f_{yy}` decides the rest: `3.0000` for {math}`\sigma_a = 0.5` (a mutant can invade, branching), `0.0000` at {math}`\sigma_a = 1`, and `-0.5556` for {math}`\sigma_a = 1.5` (uninvadable).

## Decisions

- Use `selection_gradient` to see which way the trait moves and `singular_strategy` to find where it stops; use `classify` to decide what happens there. The first two cannot tell an end point from a branching point.
- Write the fitness as a function of two Series, the mutant trait and the resident trait, that is evaluated element by element: the derivatives are taken by finite differences of that function.
- Pass a vector of residents or starting traits to treat many populations or many parameter values in one call.

## Pitfalls

A simulated walk along the gradient cannot tell the two kinds of endpoint apart. The program walks from {math}`x = 1.5` for an end point ({math}`\sigma_a = 1.5`) and for a branching point ({math}`\sigma_a = 0.5`).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the trait x is, say, beak size. Resources are richest at x = 0: carrying capacity K(x) = exp(-x^2 / 2 sk^2).
// Competition is strongest between similar individuals: alpha(y, x) = exp(-(y - x)^2 / 2 sa^2). The invasion fitness of a mutant y in a resident x is 1 - alpha K(x) / K(y).
static community::Fitness landscape(double sk, double sa) {
  return [=](const pluss::Series &mutant, const pluss::Series &resident) {
    const auto carrying = [=](const pluss::Series &x) { return x.square().mul(-0.5 / (sk * sk)).exp(); };
    const auto kernel = [=](const pluss::Series &y, const pluss::Series &x) { return y.sub(x).square().mul(-0.5 / (sa * sa)).exp(); };
    return community::logistic_competition_fitness(mutant, resident, carrying, kernel);
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // evolution by small steps along the gradient (a simple adaptive walk) started from x = 1.5, for sa = 1.5 (a true end point) and sa = 0.5 (a branching point): both walks look the same
  for (double sa : {1.5, 0.5}) {
    const auto fitness = landscape(1.0, sa);
    auto x = vec({1.5});
    std::vector<double> path;
    for (int step = 1; step <= 60; ++step) {
      x = x.add(community::selection_gradient(fitness, x).mul(0.5));
      if (step == 10 || step == 30 || step == 60) path.push_back(std::abs(flat(x)[0]));
    }
    out.writeln("competition width ", sa, ": distance of the trait from x = 0 after 10, 30, 60 steps = ", path);
  }
}
```

Output

```text
competition width 1.5000: distance of the trait from x = 0 after 10, 30, 60 steps = [0.0015, 0.0000, 0.0000]
competition width 0.5000: distance of the trait from x = 0 after 10, 30, 60 steps = [0.0015, 0.0000, 0.0000]
```

Both walks arrive at {math}`x = 0` at the same speed (`0.0015` after `10` steps, `0.0000` after `30`). A population that looked only at the gradient would conclude, wrongly for {math}`\sigma_a = 0.5`, that evolution stops there; in the branching case it is the invasion of the nearby mutants, which `classify` tests and the walk does not, that splits the population in two.

## Where next

[Biology 2: Interacting Species](../bio_02_interactions.md) introduces competition and invasion in ecological time, and [Biology 8: Genetics](../bio_08_genetics.md) follows allele frequencies under selection.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
