# Control coefficients

[Back to Biology 20: Metabolic Control](../bio_20_control.md)

In a metabolic pathway no single enzyme usually limits the flux: control is shared among the steps and shifts as conditions change. You want to know which step to change to change the output, and by how much. Metabolic control analysis answers that. This page computes the elasticities and control coefficients of a three-step pathway, shows how the control moves when one enzyme is made slower, and checks the summation theorem against a direct experiment.

## How it works

The code is in `pluss::ma::natsci::biology::metabolism::control` (`control::` below). Take the pathway {math}`S \to X_1 \to X_2 \to P` with {math}`S` held fixed and a rate {math}`v_r(x)` for each step. The elasticity {math}`\epsilon_{rs} = \partial v_r/\partial x_s` says how a rate responds to a species concentration. With the stoichiometry {math}`N` (species by reactions), the steady state obeys {math}`N v(x) = 0`, and the flux control coefficient {math}`C^J_k` is the relative response of the steady flux to a relative change in the activity of step {math}`k`. They follow from the elasticities alone,

```{math}
\begin{aligned}
&C^S = -(N\epsilon)^{-1}N\ , \\
&C^J = I + \epsilon\,C^S,
\end{aligned}
```

(with conserved moieties the same formulas hold on the independent species). In the program, `control::steady_state` finds the steady state, `control::elasticities(v, x)` differentiates the rates, and `control::coefficients(N, E)` returns `flux` and `concentration`. The summation theorem says the scaled flux coefficients add to one, and `control::scale_flux(C, v)` does the scaling.

## Elasticities and control coefficients

The pathway uses the rate constants {math}`k_1 = 2`, {math}`r_1 = 1`, {math}`k_2 = 3`, {math}`r_2 = 1` and {math}`k_3 = 1`. The program finds the steady state and prints the elasticities and both sets of coefficients.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace control = biology::metabolism::control;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a chain  S -> X1 -> X2 -> P  with reversible mass-action steps: v1 = k1 S - r1 X1, v2 = k2 X1 - r2 X2, v3 = k3 X2   (S = 1 is held fixed)
static auto chain(double k1, double r1, double k2, double r2, double k3) {
  return [=](const pluss::Series &x) {
    const auto x1 = x.select(-1, 0), x2 = x.select(-1, 1);
    const auto v1 = x1.mul(-r1).add(k1), v2 = x1.mul(k2).sub(x2.mul(r2)), v3 = x2.mul(k3);
    return v1.unsqueeze(-1).unite(std::vector<pluss::Series>{v2.unsqueeze(-1), v3.unsqueeze(-1)}, -1);
  };
}
// stoichiometry [species, reactions]
static pluss::Series stoich() { return mat({{1.0, -1.0, 0.0}, {0.0, 1.0, -1.0}}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto v = chain(2.0, 1.0, 3.0, 1.0, 1.0);
  const auto steady = control::steady_state(v, stoich(), vec({1.0, 1.0}), pluss::series::undefined(), pluss::series::undefined());
  out.writeln("steady state (X1, X2)         = ", flat(steady.state), ", converged = ", steady.converged ? "yes" : "no");
  const auto fluxes = v(steady.state);
  out.writeln("steady fluxes (v1, v2, v3)    = ", flat(fluxes));
  // elasticities: how each rate changes with each species (rows: reactions, columns: X1, X2)
  const auto e = control::elasticities(v, steady.state);
  out.writeln("elasticities (3 x 2)          = ", flat(e));
  // control coefficients: how the steady flux and the steady concentrations respond to a change in each step's rate
  const auto c = control::coefficients(stoich(), e);
  out.writeln("flux control C^J (row = flux, column = step) = ", flat(c.flux));
  out.writeln("concentration control C^S (row = X1, X2)     = ", flat(c.concentration));
}
```

Output

```text
steady state (X1, X2)         = [0.8000, 1.2000], converged = yes
steady fluxes (v1, v2, v3)    = [1.2000, 1.2000, 1.2000]
elasticities (3 x 2)          = [-1.0000, 0.0000, 3.0000, -1.0000, 0.0000, 1.0000]
flux control C^J (row = flux, column = step) = [0.6000, 0.2000, 0.2000, 0.6000, 0.2000, 0.2000, 0.6000, 0.2000, 0.2000]
concentration control C^S (row = X1, X2)     = [0.4000, -0.2000, -0.2000, 0.6000, 0.2000, -0.8000]
```

The pathway settles at {math}`X_1 = 0.8`, {math}`X_2 = 1.2` with a flux of `1.2000` in every step. The elasticities are {math}`(-1, 0)`, {math}`(3, -1)` and {math}`(0, 1)` for the three steps, and the flux control coefficients are `0.6000`, `0.2000` and `0.2000`: the first step, which supplies the pathway, has three times as much control as each of the others.

## Control moves

Control is not a property of an enzyme alone. The program makes the second enzyme slower and slower, {math}`k_2 = 30`, `3`, `0.3` and `0.03`, and prints the flux and the control each step has over it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace control = biology::metabolism::control;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a chain  S -> X1 -> X2 -> P  with reversible mass-action steps: v1 = k1 S - r1 X1, v2 = k2 X1 - r2 X2, v3 = k3 X2   (S = 1 is held fixed)
static auto chain(double k1, double r1, double k2, double r2, double k3) {
  return [=](const pluss::Series &x) {
    const auto x1 = x.select(-1, 0), x2 = x.select(-1, 1);
    const auto v1 = x1.mul(-r1).add(k1), v2 = x1.mul(k2).sub(x2.mul(r2)), v3 = x2.mul(k3);
    return v1.unsqueeze(-1).unite(std::vector<pluss::Series>{v2.unsqueeze(-1), v3.unsqueeze(-1)}, -1);
  };
}
// stoichiometry [species, reactions]
static pluss::Series stoich() { return mat({{1.0, -1.0, 0.0}, {0.0, 1.0, -1.0}}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the flux control of each step in a pathway as the second enzyme is made slower and slower: k2 = 30, 3, 0.3, 0.03
  for (double k2 : {30.0, 3.0, 0.3, 0.03}) {
    const auto v = chain(2.0, 1.0, k2, 1.0, 1.0);
    const auto steady = control::steady_state(v, stoich(), vec({1.0, 1.0}), pluss::series::undefined(), pluss::series::undefined());
    const auto c = control::coefficients(stoich(), control::elasticities(v, steady.state));
    out.writeln("k2 = ", k2, ": the flux J = ", flat(v(steady.state))[2], ", control of J by steps 1, 2, 3 = ", flat(c.flux.select(0, 2)));
  }
}
```

Output

```text
k2 = 30.0000: the flux J = 1.8750, control of J by steps 1, 2, 3 = [0.9375, 0.0312, 0.0312]
k2 = 3.0000: the flux J = 1.2000, control of J by steps 1, 2, 3 = [0.6000, 0.2000, 0.2000]
k2 = 0.3000: the flux J = 0.2609, control of J by steps 1, 2, 3 = [0.1304, 0.4348, 0.4348]
k2 = 0.0300: the flux J = 0.0296, control of J by steps 1, 2, 3 = [0.0148, 0.4926, 0.4926]
```

The first step's control falls from `0.9375` at {math}`k_2 = 30` to `0.6000`, `0.1304` and `0.0148`, while the second and third steps come to share it (`0.4926` each at {math}`k_2 = 0.03`) as the flux falls from `1.8750` to `0.0296`. Whichever step is slowest holds the control.

## Checking the theory

The scaled coefficients must add to one. The program prints them and their sum, then raises every rate constant together by one percent, which must raise the flux by exactly one percent.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace control = biology::metabolism::control;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a chain  S -> X1 -> X2 -> P  with reversible mass-action steps: v1 = k1 S - r1 X1, v2 = k2 X1 - r2 X2, v3 = k3 X2   (S = 1 is held fixed)
static auto chain(double k1, double r1, double k2, double r2, double k3) {
  return [=](const pluss::Series &x) {
    const auto x1 = x.select(-1, 0), x2 = x.select(-1, 1);
    const auto v1 = x1.mul(-r1).add(k1), v2 = x1.mul(k2).sub(x2.mul(r2)), v3 = x2.mul(k3);
    return v1.unsqueeze(-1).unite(std::vector<pluss::Series>{v2.unsqueeze(-1), v3.unsqueeze(-1)}, -1);
  };
}
// stoichiometry [species, reactions]
static pluss::Series stoich() { return mat({{1.0, -1.0, 0.0}, {0.0, 1.0, -1.0}}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // check the theory against a direct experiment: raise the activity of step 1 (k1) by 1 percent and watch the steady flux
  const auto base = chain(2.0, 1.0, 3.0, 1.0, 1.0);
  const auto s0 = control::steady_state(base, stoich(), vec({1.0, 1.0}), pluss::series::undefined(), pluss::series::undefined());
  const auto c = control::coefficients(stoich(), control::elasticities(base, s0.state));
  const double j0 = flat(base(s0.state))[2];
  // scaled coefficient: relative change of the flux per relative change of the step (here v1 is proportional to k1 for fixed X1 only through the supply term)
  const auto scaled = control::scale_flux(c.flux, base(s0.state));
  out.writeln("scaled flux control coefficients of the flux J = ", flat(scaled.select(0, 2)));
  out.writeln("sum of them (the summation theorem says 1)      = ", flat(scaled.select(0, 2).sum()));
  // the response when all three catalytic constants are raised together by 1 percent: the flux rises by 1 percent
  const auto raised = chain(2.0 * 1.01, 1.0 * 1.01, 3.0 * 1.01, 1.0 * 1.01, 1.0 * 1.01);
  const auto s1 = control::steady_state(raised, stoich(), vec({1.0, 1.0}), pluss::series::undefined(), pluss::series::undefined());
  out.writeln("relative flux change for a 1% rise of every rate constant = ", flat(raised(s1.state))[2] / j0 - 1.0);
}
```

Output

```text
scaled flux control coefficients of the flux J = [0.60000, 0.20000, 0.20000]
sum of them (the summation theorem says 1)      = [1.00000]
relative flux change for a 1% rise of every rate constant = 0.01000
```

The scaled coefficients `0.60000`, `0.20000` and `0.20000` add to `1.00000`, and the flux rises by `0.01000` when every rate constant rises by one percent: the theorem holds to the printed digits.

## Decisions

- Use `coefficients` after `steady_state` and `elasticities` at that same state; the coefficients are local, valid only at the steady state they were computed at.
- Scale with `scale_flux` when you want relative changes (a percent change of the activity gives that percent times the coefficient); use the unscaled coefficients when the rates are in different units.
- Pass the conservation laws to `steady_state` for pathways with a conserved moiety, otherwise the Jacobian is singular at the steady state.

## Pitfalls

Control coefficients are tied to the stoichiometry they came from, and to the state where they were evaluated. The program feeds `coefficients` a stoichiometry with the wrong number of reactions, and reuses the coefficients of one state at another.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace control = biology::metabolism::control;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a chain  S -> X1 -> X2 -> P  with reversible mass-action steps: v1 = k1 S - r1 X1, v2 = k2 X1 - r2 X2, v3 = k3 X2   (S = 1 is held fixed)
static auto chain(double k1, double r1, double k2, double r2, double k3) {
  return [=](const pluss::Series &x) {
    const auto x1 = x.select(-1, 0), x2 = x.select(-1, 1);
    const auto v1 = x1.mul(-r1).add(k1), v2 = x1.mul(k2).sub(x2.mul(r2)), v3 = x2.mul(k3);
    return v1.unsqueeze(-1).unite(std::vector<pluss::Series>{v2.unsqueeze(-1), v3.unsqueeze(-1)}, -1);
  };
}
// stoichiometry [species, reactions]
static pluss::Series stoich() { return mat({{1.0, -1.0, 0.0}, {0.0, 1.0, -1.0}}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // elasticities have one column per species and one row per reaction; coefficients() needs a stoichiometry that matches
  const auto v = chain(2.0, 1.0, 3.0, 1.0, 1.0);
  const auto s = control::steady_state(v, stoich(), vec({1.0, 1.0}), pluss::series::undefined(), pluss::series::undefined());
  const auto e = control::elasticities(v, s.state);
  try { (void)control::coefficients(stoich().select(1, 0).unsqueeze(1), e); } catch (const pluss::Error &err) { out.writeln("stoichiometry with one reaction: ", err.message()); }
  // coefficients of a single run are not a recipe for another: they hold only near the steady state they were computed at
  const auto v2 = chain(2.0, 1.0, 0.3, 1.0, 1.0);
  const auto c = control::coefficients(stoich(), e);
  out.writeln("control of J by step 2 computed at k2 = 3, used at k2 = 0.3: ", flat(c.flux.select(0, 2))[1]);
  const auto s2 = control::steady_state(v2, stoich(), vec({1.0, 1.0}), pluss::series::undefined(), pluss::series::undefined());
  out.writeln("control of J by step 2 recomputed at k2 = 0.3:               ", flat(control::coefficients(stoich(), control::elasticities(v2, s2.state)).flux.select(0, 2))[1]);
}
```

Output

```text
stoichiometry with one reaction: natsci: control::coefficients: N must be [S, R] and E [..., R, S]
control of J by step 2 computed at k2 = 3, used at k2 = 0.3: 0.2000
control of J by step 2 recomputed at k2 = 0.3:               0.4348
```

The first is rejected with the shapes it expects. The second runs without complaint and is wrong: step 2 has `0.2000` of the control at {math}`k_2 = 3`, but `0.4348` once {math}`k_2 = 0.3`, so recompute the coefficients whenever the parameters change.

## Where next

[Biology 25: Reaction Networks](../bio_25_networks.md) builds networks from stoichiometry and rate orders and finds their steady states, and [Biology 5: Enzymes](../bio_05_enzymes.md) gives the rate laws whose elasticities this chapter takes.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
