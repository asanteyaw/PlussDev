# Integral projection models

[Back to Biology 22: Size-Structured Populations](../bio_22_size.md)

When survival, growth and reproduction depend on a continuous size, such as the height of a tree or the length of a fish, age or stage classes lose information. An integral projection model keeps the size continuous and discretises it into a large matrix, so every tool of matrix models applies. This page builds the model for a plant, finds its growth rate and stable size distribution, computes how long a plant of each size is expected to live and its lifetime offspring, and the speed at which its seeds spread.

## How it works

The code is in `pluss::ma::natsci::biology::population::ipm` (`ipm::` below). With size {math}`z \in [0, 10]` the population changes by

```{math}
\begin{aligned}
&n_{t+1}(z') = \int K(z', z)\,n_t(z)\,\mathrm{d}z, \\
&K = s(z)\,G(z' \mid z) + f(z)\,D(z'),
\end{aligned}
```

with survival {math}`s`, growth {math}`G`, fecundity {math}`f` and offspring sizes {math}`D`. `ipm::mesh(lower, upper, n)` makes the grid, `ipm::discretize(kernel, mesh)` the matrix with the quadrature weights built in, and `ipm::analyze(A, h)` returns the growth rate {math}`\lambda`, the stable size density `w`, the reproductive values `v` and the sensitivity and elasticity kernels. Splitting the kernel into survival-growth {math}`P` and fecundity {math}`F` gives the expected remaining life {math}`\mathbf{1}^\top(I-P)^{-1}`, `ipm::life_expectancy(P)`, and the lifetime offspring, the largest eigenvalue of {math}`F(I-P)^{-1}`, `ipm::net_reproductive_rate(P, F)`. If seeds disperse with moment generating function {math}`M(s)`, the population spreads at {math}`\min_s \ln(R_0 M(s))/s`, which `ipm::spreading_speed(ln_r0, mgf, s_max)` finds.

## Building and analysing the kernel

Survival rises with size as a logistic, growth is Gaussian about {math}`0.9z + 0.7`, and larger plants produce more seedlings that start small. The program discretises the kernel on `100` points and prints the growth rate and the stable size distribution's mean and variance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace ipm = population::ipm;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // plants classified by size z in [0, 10]. Survival rises with size (logistic), growth is Gaussian about 0.9 z + 0.7, and the plants produce seedlings that start small
  const auto m = ipm::mesh(0.0, 10.0, 100);
  const ipm::Kernel kernel = [](const pluss::Series &zp, const pluss::Series &z) {
    const auto survival = z.sub(3.0).mul(-1.0).exp().add(1.0).reciprocal();                 // 1 / (1 + exp(-(z - 3)))
    const auto growth = ipm::gaussian_density(zp, z.mul(0.9).add(0.7), z.mul(0.0).add(0.8));
    const auto fecundity = z.mul(0.15).mul(ipm::gaussian_density(zp, z.mul(0.0).add(1.0), z.mul(0.0).add(0.5)));   // bigger plants make more seedlings
    return survival.mul(growth).add(fecundity);
  };
  const auto a = ipm::discretize(kernel, m);
  const auto result = ipm::analyze(a, m.h);
  out.writeln("growth rate lambda        = ", flat(result.lambda));
  out.writeln("mean size of the stable population = ", flat(ipm::mean_size(result.w, m)));
  out.writeln("variance of the stable sizes       = ", flat(ipm::variance_size(result.w, m)));
  out.writeln("converged = ", result.converged ? "yes" : "no");
}
```

Output

```text
growth rate lambda        = [0.9515]
mean size of the stable population = [3.3806]
variance of the stable sizes       = [8.8178]
converged = yes
```

The population is declining, {math}`\lambda = 0.9515`. Its stable size distribution has mean `3.3806` and variance `8.8178`, and the eigen-solver reports convergence.

## Lifetimes and reproduction

Now split the kernel. The program prints the expected remaining life at five sizes, the net reproductive rate, and checks that the elasticities of {math}`\lambda` sum to one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace ipm = population::ipm;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto m = ipm::mesh(0.0, 10.0, 100);
  const ipm::Kernel survival_growth = [](const pluss::Series &zp, const pluss::Series &z) {
    return z.sub(3.0).mul(-1.0).exp().add(1.0).reciprocal().mul(ipm::gaussian_density(zp, z.mul(0.9).add(0.7), z.mul(0.0).add(0.8)));
  };
  const ipm::Kernel fecundity = [](const pluss::Series &zp, const pluss::Series &z) {
    return z.mul(0.15).mul(ipm::gaussian_density(zp, z.mul(0.0).add(1.0), z.mul(0.0).add(0.5)));
  };
  const auto p = ipm::discretize(survival_growth, m), f = ipm::discretize(fecundity, m);
  // how long a plant of each size is expected to live (in years), and the net reproductive rate
  const auto life = flat(ipm::life_expectancy(p));
  out.writeln("life expectancy of a plant of size 0.5, 2.5, 5, 7.5, 9.5 = ", std::vector<double>{life[5], life[25], life[50], life[75], life[95]});
  out.writeln("net reproductive rate R0 = ", flat(ipm::net_reproductive_rate(p, f)));
  // lambda of the full model and the elasticity of lambda to survival-growth versus fecundity
  const auto a = p.add(f);
  const auto result = ipm::analyze(a, m.h);
  const auto el = result.elasticity;
  out.writeln("lambda = ", flat(result.lambda));
  out.writeln("total elasticity (should be 1) = ", flat(el.sum().mul(m.h * m.h)));
}
```

Output

```text
life expectancy of a plant of size 0.5, 2.5, 5, 7.5, 9.5 = [1.1133, 2.7612, 13.1891, 20.1283, 14.5659]
net reproductive rate R0 = [0.3012]
lambda = [0.9515]
total elasticity (should be 1) = [1.0000]
```

A plant of size `0.5` is expected to live `1.1133` more years and one of size `7.5` another `20.1283`; the lifetime offspring is `0.3012`, below one, which fits the decline. The elasticities sum to `1.0000` as they must.

## How fast seeds spread

For Gaussian dispersal with standard deviation {math}`\sigma` the speed has the closed form {math}`\sigma\sqrt{2\ln R_0}`. The program computes the speed for {math}`R_0 = 1.5`, `3` and `10` with {math}`\sigma = 2` and prints the formula beside it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace ipm = population::ipm;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // seeds disperse by a kernel with a moment generating function M(s); the spreading speed of the population is min over s of ln(R0 M(s)) / s
  // Gaussian dispersal with standard deviation 2: M(s) = exp(s^2 sd^2 / 2); R0 = 1.5, 3 and 10
  const auto ln_r0 = vec({std::log(1.5), std::log(3.0), std::log(10.0)});
  const auto mgf = [](const pluss::Series &s) { return s.square().mul(2.0).exp(); };
  out.writeln("spreading speed (distance per generation) = ", flat(ipm::spreading_speed(ln_r0, mgf, 5.0)));
  out.writeln("closed form sd sqrt(2 ln R0)              = ", std::vector<double>{2.0 * std::sqrt(2.0 * std::log(1.5)), 2.0 * std::sqrt(2.0 * std::log(3.0)), 2.0 * std::sqrt(2.0 * std::log(10.0))});
}
```

Output

```text
spreading speed (distance per generation) = [1.8010, 2.9646, 4.2919]
closed form sd sqrt(2 ln R0)              = [1.8010, 2.9646, 4.2919]
```

The numerical speeds `1.8010`, `2.9646` and `4.2919` per generation equal the closed form to the digits printed.

## Decisions

- Use `ipm` instead of a stage matrix when the vital rates are smooth functions of size; the number of mesh points only controls the accuracy of the quadrature, so increase it until {math}`\lambda` stops changing.
- Take life expectancy and net reproductive rate from the split kernels {math}`P` and {math}`F`; do not read them from the full matrix, which mixes survival with reproduction.
- Choose the upper bound of the mesh above every size a plant can reach, with margin for the growth variance.

## Pitfalls

A mesh needs distinct bounds and at least two points, a model whose mesh stops short loses plants, and a population with {math}`R_0 \le 1` has no spreading speed. The program tries each; for the loss it discretises a kernel in which every plant survives and grows to about {math}`0.9z + 1`, and prints the share of offspring that land on the mesh.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace ipm = population::ipm;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  try { (void)ipm::mesh(0.0, 10.0, 1); } catch (const pluss::Error &e) { out.writeln("one mesh point:  ", e.message()); }
  try { (void)ipm::mesh(10.0, 0.0, 50); } catch (const pluss::Error &e) { out.writeln("reversed bounds: ", e.message()); }
  // the mesh must reach every size the kernel sends plants to: plants that would grow past the upper bound simply disappear from the model
  const ipm::Kernel growth = [](const pluss::Series &zp, const pluss::Series &z) {
    return ipm::gaussian_density(zp, z.mul(0.9).add(1.0), z.mul(0.0).add(0.8));   // every plant survives and grows to about 0.9 z + 1
  };
  for (double upper : {4.0, 10.0}) {
    const auto m = ipm::mesh(0.0, upper, 100);
    const auto a = ipm::discretize(growth, m);
    const auto kept = flat(a.sum(0));          // the share of each column's offspring that lands on the mesh
    out.writeln("mesh up to ", upper, ": a plant of size 0.5 keeps ", kept[std::int64_t(0.5 / upper * 100)], " of its offspring on the mesh, a plant of size 3 keeps ", kept[std::int64_t(3.0 / upper * 100)]);
  }
  // a population with R0 of one or less does not spread, and the speed formula has no minimum to find
  const auto mgf = [](const pluss::Series &s) { return s.square().mul(2.0).exp(); };
  try { (void)ipm::spreading_speed(vec({std::log(0.5)}), mgf, 5.0); } catch (const pluss::Error &e) { out.writeln("R0 = 0.5: ", e.message()); }
}
```

Output

```text
one mesh point:  natsci: ipm::mesh: need upper > lower and at least two points
reversed bounds: natsci: ipm::mesh: need upper > lower and at least two points
mesh up to 4.0000: a plant of size 0.5 keeps 0.9643 of its offspring on the mesh, a plant of size 3 keeps 0.6378
mesh up to 10.0000: a plant of size 0.5 keeps 0.9693 of its offspring on the mesh, a plant of size 3 keeps 1.0000
R0 = 0.5: natsci: ipm::spreading_speed: ln R0 must be positive (a population with R0 <= 1 does not spread)
```

The first two are errors. The third shows the quiet one: on a mesh to `4`, only `0.6378` of a size-3 plant's offspring land on the mesh, the rest disappear, while on a mesh to `10` all of them do (`1.0000`); the small deficit at size `0.5` is mass that falls below size zero. The last is an error again: with {math}`R_0 = 0.5` the population does not invade, so no speed is returned.

## Where next

[Biology 3: Demography and Extinction](../bio_03_demography.md) has the matrix models that this chapter makes continuous, and [Biology 23: Census Data](../bio_23_census.md) turns to the data that such models are fitted to.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
