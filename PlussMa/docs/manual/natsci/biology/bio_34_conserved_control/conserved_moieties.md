# Conservation laws and control

[Back to Biology 34: Control With Conserved Moieties](../bio_34_conserved_control.md)

Many metabolic networks conserve a quantity: ATP plus ADP, NAD plus NADH, the total of an enzyme in all its forms. These conservation laws change what steady state means, and without them the control coefficients of [Biology 20](../bio_20_control/control_coefficients.md) cannot be computed. This page finds the conservation laws of a network, finds its steady state under a given total, computes the scaled control coefficients, tests them against the theorems that must hold, and turns a parameter change into a response of the steady state.

## How it works

With stoichiometry $N$ (species by reactions), the concentrations obey $\dot x = N v(x)$. A *conservation law* is a row $\gamma$ with $\gamma N = 0$: the combination $\gamma x$ never changes. The conservation laws form a matrix $\Gamma$ of $C$ rows, the left null space of $N$, and the species can change only along the *free directions* $B$, the null space of $\Gamma$. `control::conservation_laws(N)` returns an orthonormal $\Gamma$ (with no rows when nothing is conserved) and `control::free_directions(Gamma)` returns $B$. The steady state solves $N v(x) = 0$ together with $\Gamma x = T$ for the totals $T$, which `control::steady_state(v, N, x0, Gamma, T)` finds by damped Newton iteration from a positive guess. At the steady state the unscaled control coefficients solve

```{math}
\begin{aligned}
&N\,E\,C^S = -N, \\
&\Gamma\,C^S = 0, \\
&C^J = I + E\,C^S
\end{aligned}
```

where $E = \partial v/\partial x$ holds the elasticities, and `control::coefficients(N, E, Gamma)` returns $C^S$ and $C^J$. They are scaled by `scale_elasticities`, `scale_concentration` and `scale_flux`. Two theorems audit them: the summation theorem says the scaled flux coefficients of any flux add to one and those of any concentration to zero, and the connectivity theorem says $C^J E B = 0$ and $C^S E B = -B$. `control::summation_defect` and `control::connectivity_defect` return the largest violation, which should be rounding error. For the response to a parameter $p$, `control::parameter_derivatives(v, x, p)` gives $\partial v/\partial p$ and `control::response(C, dv_dp)` gives the response coefficient $R = C\,\partial v/\partial p$. The programs use an adenylate-like pair: A (charged) and B (discharged), with $A + B$ conserved, a charging reaction $v_1 = k_1 B$ and two discharging reactions $v_2 = k_2 A$ and $v_3 = k_3 A$.

## Finding the conservation laws

The program finds the conservation laws of the pair, checks that they annihilate the stoichiometry, finds the free directions, and counts the laws of an open chain for comparison.

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
// an adenylate-like pair of species: A (charged) and B (discharged), A + B = T always. Reaction 1 charges B -> A at rate k1 B, reactions 2 and 3
// discharge A -> B at rates k2 A and k3 A. Stoichiometry [species, reactions]
static pluss::Series stoich() { return mat({{1.0, -1.0, -1.0}, {-1.0, 1.0, 1.0}}); }
static auto rates(double k1, double k2, double k3) {
  return [=](const pluss::Series &x) {
    const auto a = x.select(-1, 0), b = x.select(-1, 1);
    return b.mul(k1).unsqueeze(-1).unite(std::vector<pluss::Series>{a.mul(k2).unsqueeze(-1), a.mul(k3).unsqueeze(-1)}, -1);
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto n = stoich();
  // the conservation laws are the left null space of N: combinations of species that never change
  const auto gamma = control::conservation_laws(n);
  out.writeln("number of conservation laws ", std::vector<double>{static_cast<double>(gamma.size(0))});
  out.writeln("Gamma (one row, normalised, sign arbitrary) ", flat(gamma.abs()));
  out.writeln("Gamma N (should be 0)       ", flat(gamma.matmul(n).abs()));
  // the free directions are the species changes that the conservation laws allow
  const auto free = control::free_directions(gamma);
  out.writeln("free directions (one column) ", flat(free.abs()));
  out.writeln("Gamma times free directions  ", flat(gamma.matmul(free).abs()));
  // a pathway with no conservation: S -> X1 -> X2 -> P has no moiety conserved, so Gamma has no rows
  const auto open = control::conservation_laws(mat({{1.0, -1.0, 0.0}, {0.0, 1.0, -1.0}}));
  out.writeln("laws of the open chain      ", std::vector<double>{static_cast<double>(open.size(0))});
}
```

Output

```text
number of conservation laws [1.0000]
Gamma (one row, normalised, sign arbitrary) [0.7071, 0.7071]
Gamma N (should be 0)       [0.0000, 0.0000, 0.0000]
free directions (one column) [0.7071, 0.7071]
Gamma times free directions  [0.0000]
laws of the open chain      [0.0000]
```

There is `1` conservation law, $\Gamma = (0.7071, 0.7071)$ up to a sign, which is the total $A + B$ divided by $\sqrt 2$, and $\Gamma N = 0$ in all three columns. The one free direction has entries of size `0.7071`, the direction $(1, -1)/\sqrt 2$ in which A can grow only by B shrinking, and $\Gamma B = 0$. The open chain $S \to X_1\to X_2\to P$ has no conservation law at all, `0` rows.

## The steady state under a total

The program sets $k_1 = 3$, $k_2 = k_3 = 1$, a total $A + B = 10$ (in units of $\Gamma$, the total is $10/\sqrt2$), and finds the steady state and its fluxes.

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
// an adenylate-like pair of species: A (charged) and B (discharged), A + B = T always. Reaction 1 charges B -> A at rate k1 B, reactions 2 and 3
// discharge A -> B at rates k2 A and k3 A. Stoichiometry [species, reactions]
static pluss::Series stoich() { return mat({{1.0, -1.0, -1.0}, {-1.0, 1.0, 1.0}}); }
static auto rates(double k1, double k2, double k3) {
  return [=](const pluss::Series &x) {
    const auto a = x.select(-1, 0), b = x.select(-1, 1);
    return b.mul(k1).unsqueeze(-1).unite(std::vector<pluss::Series>{a.mul(k2).unsqueeze(-1), a.mul(k3).unsqueeze(-1)}, -1);
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto n = stoich();
  const auto v = rates(3.0, 1.0, 1.0);          // charge k1 = 3, discharge k2 = 1 and k3 = 1
  const auto gamma = control::conservation_laws(n);
  // the steady state with the total A + B = 10 as the conservation constraint
  const auto steady = control::steady_state(v, n, vec({5.0, 5.0}), gamma, vec({10.0 / std::sqrt(2.0)}));
  out.writeln("steady state (A, B) ", flat(steady.state), "  converged ", steady.converged ? "yes" : "no");
  out.writeln("sum A + B           ", std::vector<double>{flat(steady.state)[0] + flat(steady.state)[1]});
  out.writeln("fluxes (v1, v2, v3) ", flat(v(steady.state)));
}
```

Output

```text
steady state (A, B) [6.0000, 4.0000]  converged yes
sum A + B           [10.0000]
fluxes (v1, v2, v3) [12.0000, 6.0000, 6.0000]
```

The steady state is $(A, B) = (6, 4)$, converged, with $A + B = 10$ as required. The fluxes are $(12, 6, 6)$: the charging flux $k_1 B = 12$ equals the two discharging fluxes $6 + 6$, as the steady state demands.

## Control coefficients and the theorems

The program computes the elasticities and the control coefficients with the conservation matrix, scales them, and prints the control of the first flux, the control of A, the scaled elasticities and the two defects.

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
// an adenylate-like pair of species: A (charged) and B (discharged), A + B = T always. Reaction 1 charges B -> A at rate k1 B, reactions 2 and 3
// discharge A -> B at rates k2 A and k3 A. Stoichiometry [species, reactions]
static pluss::Series stoich() { return mat({{1.0, -1.0, -1.0}, {-1.0, 1.0, 1.0}}); }
static auto rates(double k1, double k2, double k3) {
  return [=](const pluss::Series &x) {
    const auto a = x.select(-1, 0), b = x.select(-1, 1);
    return b.mul(k1).unsqueeze(-1).unite(std::vector<pluss::Series>{a.mul(k2).unsqueeze(-1), a.mul(k3).unsqueeze(-1)}, -1);
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto n = stoich();
  const auto v = rates(3.0, 1.0, 1.0);
  const auto gamma = control::conservation_laws(n);
  const auto steady = control::steady_state(v, n, vec({5.0, 5.0}), gamma, vec({10.0 / std::sqrt(2.0)}));
  const auto x = steady.state, flux = v(x);
  const auto e = control::elasticities(v, x);
  // with the conservation law the coefficients need Gamma as the third argument; without it N E is singular
  const auto c = control::coefficients(n, e, gamma);
  const auto cj = control::scale_flux(c.flux, flux);
  const auto cs = control::scale_concentration(c.concentration, x, flux);
  out.writeln("scaled flux control of reaction 1 (rows: fluxes, columns: reactions) ", flat(cj.select(0, 0)));
  out.writeln("scaled concentration control of A (columns: reactions)            ", flat(cs.select(0, 0)));
  out.writeln("scaled elasticities (reactions x species) ", flat(control::scale_elasticities(e, x, flux)));
  // the summation theorem: scaled flux coefficients add to 1 and scaled concentration coefficients to 0
  out.writeln("summation defect  ", flat(control::summation_defect(cj, cs)));
  // the connectivity theorem on the free directions
  out.writeln("connectivity defect ", flat(control::connectivity_defect(c, e, control::free_directions(gamma))));
}
```

Output

```text
scaled flux control of reaction 1 (rows: fluxes, columns: reactions) [0.4000, 0.3000, 0.3000]
scaled concentration control of A (columns: reactions)            [0.4000, -0.2000, -0.2000]
scaled elasticities (reactions x species) [0.0000, 1.0000, 1.0000, 0.0000, 1.0000, 0.0000]
summation defect  [0.0000]
connectivity defect [0.0000]
```

The first flux is controlled `0.4` by the charging step and `0.3` by each discharging step, which add to `1` as the summation theorem requires. The concentration of A responds `+0.4` to the charging step and `-0.2` to each discharging step, adding to `0`. The scaled elasticities $(0, 1; 1, 0; 1, 0)$ say that the charging rate depends only on B and the two discharging rates only on A, each with a power law of order one. The summation and the connectivity defects are `0.0000`.

## From parameters to responses

The program computes $\partial v/\partial p$ for the parameters $(k_1, k_2, k_3)$, multiplies by the concentration control to get the response of A and B to each parameter, and checks the first number by changing $k_1$ and solving again.

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
// an adenylate-like pair of species: A (charged) and B (discharged), A + B = T always. Reaction 1 charges B -> A at rate k1 B, reactions 2 and 3
// discharge A -> B at rates k2 A and k3 A. Stoichiometry [species, reactions]
static pluss::Series stoich() { return mat({{1.0, -1.0, -1.0}, {-1.0, 1.0, 1.0}}); }
static auto rates(double k1, double k2, double k3) {
  return [=](const pluss::Series &x) {
    const auto a = x.select(-1, 0), b = x.select(-1, 1);
    return b.mul(k1).unsqueeze(-1).unite(std::vector<pluss::Series>{a.mul(k2).unsqueeze(-1), a.mul(k3).unsqueeze(-1)}, -1);
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto n = stoich();
  const auto gamma = control::conservation_laws(n);
  const double total = 10.0 / std::sqrt(2.0);  // A + B = 10 in the (non-normalised) units
  // rates with parameters p = (k1, k2, k3) as an argument
  const auto vp = [](const pluss::Series &x, const pluss::Series &p) {
    const auto a = x.select(-1, 0), b = x.select(-1, 1);
    return b.mul(p.select(-1, 0)).unsqueeze(-1).unite(std::vector<pluss::Series>{a.mul(p.select(-1, 1)).unsqueeze(-1), a.mul(p.select(-1, 2)).unsqueeze(-1)}, -1);
  };
  const auto p = vec({3.0, 1.0, 1.0});
  const auto v = rates(3.0, 1.0, 1.0);
  const auto steady = control::steady_state(v, n, vec({5.0, 5.0}), gamma, vec({total}));
  const auto x = steady.state;
  const auto dvdp = control::parameter_derivatives(vp, x, p);
  out.writeln("dv/dp (rows: reactions, columns: k1, k2, k3) ", flat(dvdp));
  const auto c = control::coefficients(n, control::elasticities(v, x), gamma);
  // the response of the steady state to each parameter: R = C dv/dp
  const auto response = control::response(c.concentration, dvdp);
  out.writeln("d(A, B) / d(k1, k2, k3) (rows A and B) ", flat(response));
  // check by changing k1 by a little and solving again
  const auto v2 = rates(3.0 + 1e-5, 1.0, 1.0);
  const auto again = control::steady_state(v2, n, vec({5.0, 5.0}), gamma, vec({total}));
  out.writeln("finite-difference dA/dk1 ", std::vector<double>{(flat(again.state)[0] - flat(x)[0]) / 1e-5});
}
```

Output

```text
dv/dp (rows: reactions, columns: k1, k2, k3) [4.0000, 0.0000, 0.0000, 0.0000, 6.0000, 0.0000, 0.0000, 0.0000, 6.0000]
d(A, B) / d(k1, k2, k3) (rows A and B) [0.8000, -1.2000, -1.2000, -0.8000, 1.2000, 1.2000]
finite-difference dA/dk1 [0.8000]
```

The derivative matrix is diagonal, `(4, 6, 6)`: $\partial v_1/\partial k_1 = B = 4$ and $\partial v_2/\partial k_2 = \partial v_3/\partial k_3 = A = 6$. The responses of A are `0.8`, `-1.2`, `-1.2`, and those of B are the same with the opposite sign, since $A + B$ is constant. The finite-difference check, a small change of $k_1$ and a new steady state, gives $\partial A/\partial k_1 = 0.8000$.

## Decisions

- Compute $\Gamma$ with `conservation_laws` instead of writing it by hand: it is orthonormal, which is what the solvers expect, and it finds the laws you did not notice.
- Pass the totals in the units of $\Gamma$: with an orthonormal row, the number you want to conserve is divided by the row's norm, as the program shows.
- Run `summation_defect` and `connectivity_defect` on every set of coefficients you report: they cost nothing and catch wrong elasticities.
- Use `parameter_derivatives` with your rate law as a function of $(x, p)$ rather than re-deriving $\partial v/\partial p$ by hand.

## Pitfalls

Forgetting the conservation law, a starting guess with a zero concentration, and elasticities of the wrong shape are all refused. The program tries each.

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
// an adenylate-like pair of species: A (charged) and B (discharged), A + B = T always. Reaction 1 charges B -> A at rate k1 B, reactions 2 and 3
// discharge A -> B at rates k2 A and k3 A. Stoichiometry [species, reactions]
static pluss::Series stoich() { return mat({{1.0, -1.0, -1.0}, {-1.0, 1.0, 1.0}}); }
static auto rates(double k1, double k2, double k3) {
  return [=](const pluss::Series &x) {
    const auto a = x.select(-1, 0), b = x.select(-1, 1);
    return b.mul(k1).unsqueeze(-1).unite(std::vector<pluss::Series>{a.mul(k2).unsqueeze(-1), a.mul(k3).unsqueeze(-1)}, -1);
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto n = stoich();
  const auto v = rates(3.0, 1.0, 1.0);
  const auto gamma = control::conservation_laws(n);
  const auto steady = control::steady_state(v, n, vec({5.0, 5.0}), gamma, vec({10.0 / std::sqrt(2.0)}));
  const auto e = control::elasticities(v, steady.state);
  // forgetting the conservation law: N E is singular
  try {
    const auto c = control::coefficients(n, e);
    out.writeln("without Gamma: largest entry of C^S ", std::vector<double>{c.concentration.abs().max().item<double>()});
  } catch (const pluss::Error &e2) {
    out.writeln("without Gamma: ", e2.message());
  }
  // a starting guess with a zero concentration
  try {
    control::steady_state(v, n, vec({0.0, 5.0}), gamma, vec({10.0 / std::sqrt(2.0)}));
  } catch (const pluss::Error &e2) {
    out.writeln("zero concentration in the guess: ", e2.message());
  }
  // shapes that do not fit
  try {
    control::coefficients(n, pluss::series::zeros({3, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)), gamma);
  } catch (const pluss::Error &e2) {
    out.writeln("elasticities of the wrong shape: ", e2.message());
  }
}
```

Output

```text
without Gamma: natsci: control::coefficients: the network has moiety conservation laws that were not passed (use control::conservation_laws(N)), so N E is singular
zero concentration in the guess: natsci: control::steady_state: initial guess must be strictly positive
elasticities of the wrong shape: natsci: control::coefficients: N must be [S, R] and E [..., R, S]
```

Without $\Gamma$ the matrix $N E$ is singular for this network, and the call is refused with a message that names the missing conservation laws and the function that finds them. A zero in the guess is refused because the steady-state iteration needs strictly positive concentrations, and a shape error names the shapes that were expected.

## Where next

Go back to [Biology 20: Control Coefficients](../bio_20_control/control_coefficients.md) for the pathway without conservation, and on to [Biology 25: Reaction Networks](../bio_25_networks.md), whose steady states come with conservation laws too.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
