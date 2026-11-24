# Stage matrices

[Back to Biology 3: Demography and Extinction](../bio_03_demography.md)

When individuals differ by stage, a population is a vector and a year of life is a matrix. This page takes a three-stage population (juvenile, subadult, adult), reads its long-run growth rate, stage mix and reproductive values off the matrix, ranks which entries matter most, and computes lifetime quantities and a parameter sensitivity.

## How it works

Column `j` of the projection matrix holds what a stage-`j` individual becomes in one year: the fertility entries in the top row are offspring, the others are survival and moves between stages. `structured::dominant(a)` returns the dominant eigenvalue `lambda`, the long-run yearly growth factor; `right`, the stable stage mix that sums to 1; and `left`, the reproductive values, which say how much one individual of each stage contributes to the future. `structured::elasticity(a, e)` returns, entry by entry, the proportional effect on `lambda` of a proportional change in that entry; the elasticities sum to 1. `demography::damping_ratio` is how fast a population settles onto the stable mix. Splitting the matrix into fertilities `F` and transitions `T`, `fundamental_matrix(T)` gives the expected time an individual spends in each stage and `net_reproductive_rate(a, F)` the offspring it leaves in a lifetime. `lambda_gradient` differentiates `lambda` with respect to parameters that the matrix is built from.

## Reading a matrix

The program builds the matrix, prints its dominant eigen-quantities, the elasticities (row by row), the damping ratio and the lifetime quantities, then differentiates `lambda` with respect to the adult survival entry at `0.8`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace population = biology::population;
namespace structured = population::structured;
namespace demography = population::demography;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // three stages: juvenile, subadult, adult. Column j holds what a stage-j individual becomes in one year.
  const auto a = mat({{0.0, 0.0, 4.0}, {0.4, 0.3, 0.0}, {0.0, 0.5, 0.8}});
  const auto e = structured::dominant(a);
  out.writeln("growth rate lambda    = ", flat(e.lambda));
  out.writeln("stable stage mix      = ", flat(e.right));
  out.writeln("reproductive value    = ", flat(e.left));
  out.writeln("elasticities (rows):  ", flat(structured::elasticity(a, e)));
  out.writeln("damping ratio         = ", flat(demography::damping_ratio(a)));

  // lifetime quantities: split A into fertilities F and transitions T
  const auto fertility = mat({{0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}});
  out.writeln("net reproductive rate = ", flat(demography::net_reproductive_rate(a, fertility)));
  const auto transitions = a.sub(a.mul(fertility));
  const auto fundamental = demography::fundamental_matrix(transitions);
  out.writeln("expected lifetime (years, from each stage) = ", flat(fundamental.expected_lifetime));

  // how does lambda respond to adult survival? the parameter is the adult-to-adult entry
  const auto matrix_of = [&](const pluss::Series &theta) {
    const auto base = mat({{0.0, 0.0, 4.0}, {0.4, 0.3, 0.0}, {0.0, 0.5, 0.0}}).to(theta.scalar_type());
    const auto adult = mat({{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}}).to(theta.scalar_type());
    return base.add(adult.mul(theta.select(-1, 0).unsqueeze(-1).unsqueeze(-1)));
  };
  out.writeln("d lambda / d adult survival = ", flat(demography::lambda_gradient(matrix_of, vec({0.8}))));
}
```

Output

```text
growth rate lambda    = [1.3574]
stable stage mix      = [0.5822, 0.2202, 0.1976]
reproductive value    = [0.3640, 1.2352, 2.6121]
elasticities (rows):  [0.0000, 0.0000, 0.2119, 0.2119, 0.0601, 0.0000, 0.0000, 0.2119, 0.3041]
damping ratio         = [1.7681]
net reproductive rate = [5.7143]
expected lifetime (years, from each stage) = [3.0000, 5.0000, 5.0000]
d lambda / d adult survival = [0.5161]
```

The population grows by a factor `1.3574` a year with a stable mix of `58%` juveniles, `22%` subadults and `20%` adults. The reproductive values are `0.3640`, `1.2352` and `2.6121`, so an adult contributes about seven times as much to the future as a juvenile. The elasticities add to `1`, and the largest, `0.3041`, belongs to adult survival, with `0.2119` for each of the entries on the main life path. The damping ratio `1.7681` says that the deviation from the stable mix shrinks by about that factor each year. Each individual leaves `5.7143` offspring in its lifetime; the expected lifetime is `3`, `5` and `5` years from the three stages, and `lambda` rises by `0.5161` for each unit rise in adult survival.

<!-- addex:start -->
## An age-structured matrix

When the stages are age classes the projection matrix has a special form, the Leslie matrix, with fertilities along the first row and survival probabilities just below the diagonal. `structured::leslie(fertility, survival)` builds it, and `structured::project(matrix, n, steps)` advances a population vector by that many years. The program builds a four-class matrix, prints its growth rate, and projects `100` newborns for twelve years while printing the yearly growth ratio.

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

namespace structured = population::structured;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // an age-structured (Leslie) matrix from fertilities and survival: age classes 0, 1, 2, 3
  const auto fertility = vec({0.0, 1.2, 2.5, 1.0});
  const auto survival = vec({0.6, 0.7, 0.4});
  const auto leslie = structured::leslie(fertility, survival);
  out.writeln("Leslie matrix rows (flattened) = ", flat(leslie));
  const auto e = structured::dominant(leslie);
  out.writeln("long-run growth rate lambda    = ", flat(e.lambda));
  // project 100 individuals, all newborn, forward and watch the yearly growth ratio approach lambda
  auto n = vec({100.0, 0.0, 0.0, 0.0});
  double previous = 100.0;
  for (int year = 1; year <= 12; ++year) {
    n = structured::project(leslie, n, 1);
    const double total = flat(n.sum())[0];
    if (year == 1 || year == 4 || year == 8 || year == 12) out.writeln("year ", year, ": total = ", total, ", growth over the year = ", total / previous);
    previous = total;
  }
}
```

Output

```text
Leslie matrix rows (flattened) = [0.0000, 1.2000, 2.5000, 1.0000, 0.6000, 0.0000, 0.0000, 0.0000, 0.0000, 0.7000, 0.0000, 0.0000, 0.0000, 0.0000, 0.4000, 0.0000]
long-run growth rate lambda    = [1.2813]
year 1: total = 60.0000, growth over the year = 0.6000
year 4: total = 161.8800, growth over the year = 0.9811
year 8: total = 510.6529, growth over the year = 1.3557
year 12: total = 1341.8839, growth over the year = 1.2679
```

The long-run growth rate is `1.2813`. The population does not reach it at once: the yearly growth ratio is `0.6000` in the first year, `0.9811` in the fourth, `1.3557` in the eighth and `1.2679` in the twelfth, overshooting and then approaching `1.2813` as the age distribution settles. The first year is low because the newborns have not yet reached the ages at which they breed.

## Transient growth

A population can grow, or shrink, much faster than {math}`\lambda` for a while before it settles. `demography::amplification(matrix, horizon)` returns the largest factor by which the population can exceed its long-run trajectory, the year at which that happens, and the factor after the first year. The program compares a matrix with a long delay before breeding, and a three-stage cycle in which breeding happens only at the last stage.

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

namespace demography = population::demography;
namespace structured = population::structured;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a population can first grow or shrink faster than lambda before it settles: transient amplification
  const auto matrix = mat({{0.0, 6.0}, {0.05, 0.0}});   // newborns are produced by adults, but few survive to be adults
  const auto big = mat({{0.0, 0.0, 12.0}, {0.9, 0.0, 0.0}, {0.0, 0.95, 0.0}});
  out.writeln("lambda, matrix 1 = ", flat(structured::dominant(matrix).lambda));
  const auto a1 = demography::amplification(matrix, 10);
  out.writeln("first-step amplification = ", flat(a1.first_step), ", largest within 10 years = ", flat(a1.maximum), " at year ", flat(a1.time_of_maximum));
  out.writeln("lambda, matrix 2 = ", flat(structured::dominant(big).lambda));
  const auto a2 = demography::amplification(big, 10);
  out.writeln("first-step amplification = ", flat(a2.first_step), ", largest within 10 years = ", flat(a2.maximum), " at year ", flat(a2.time_of_maximum));
  out.writeln("damping ratio of matrix 2 = ", flat(demography::damping_ratio(big)));
}
```

Output

```text
lambda, matrix 1 = [0.5477]
first-step amplification = [10.9545], largest within 10 years = [10.9545] at year [1.0000]
lambda, matrix 2 = [2.1729]
first-step amplification = [5.5225], largest within 10 years = [5.5225] at year [10.0000]
damping ratio of matrix 2 = [1.0000]
```

For the first matrix {math}`\lambda = 0.5477`, so the population is declining in the long run, yet in the first year a population vector can be `10.9545` times larger than the long-run curve would give. For the second, {math}`\lambda = 2.1729` and the amplification reaches `5.5225`. Its damping ratio is exactly `1.0000`: a pure three-stage cycle has no stable stage mix, so the population never settles onto one and the transient never dies out.

## Which vital rate matters most

`demography::lambda_gradient(matrix_fn, theta)` differentiates {math}`\lambda` with respect to the parameters that the matrix is built from, whatever shape the dependence takes. The program builds a two-stage matrix from a juvenile survival {math}`s_j` and an adult survival {math}`s_a`, and prints the sensitivities of {math}`\lambda` at one point, then at three adult survivals in a single batched call.

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

namespace demography = population::demography;
namespace structured = population::structured;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // which vital rate matters most? the matrix depends on a juvenile survival s_j and an adult survival s_a; lambda_gradient differentiates lambda with respect to both
  const auto matrix_of = [](const pluss::Series &theta) {
    const auto sj = theta.select(-1, 0).unsqueeze(-1).unsqueeze(-1), sa = theta.select(-1, 1).unsqueeze(-1).unsqueeze(-1);
    const auto jx = mat({{0.0, 0.0}, {1.0, 0.0}}).to(theta.scalar_type()), ax = mat({{0.0, 0.0}, {0.0, 1.0}}).to(theta.scalar_type());
    const auto births = mat({{0.0, 1.5}, {0.0, 0.0}}).to(theta.scalar_type());
    return births.add(jx.mul(sj)).add(ax.mul(sa));
  };
  const auto theta = vec({0.4, 0.8});
  const auto grad = demography::lambda_gradient(matrix_of, theta);
  out.writeln("lambda at (s_j = 0.4, s_a = 0.8) = ", flat(structured::dominant(matrix_of(theta)).lambda));
  out.writeln("d lambda / d s_j, d lambda / d s_a = ", flat(grad));
  // the same gradient for several adult survivals in one batch
  const auto batch = vec({0.4, 0.5, 0.4, 0.8, 0.4, 0.95}).reshape({3, 2});
  out.writeln("batched gradients (s_j, s_a per row) = ", flat(demography::lambda_gradient(matrix_of, batch)));
}
```

Output

```text
lambda at (s_j = 0.4, s_a = 0.8) = [1.2718]
d lambda / d s_j, d lambda / d s_a = [0.8603, 0.7294]
batched gradients (s_j, s_a per row) = [0.9214, 0.6536, 0.8603, 0.7294, 0.8254, 0.7614]
```

At {math}`s_j = 0.4`, {math}`s_a = 0.8` the growth rate is `1.2718` and its sensitivities are `0.8603` to juvenile survival and `0.7294` to adult survival: a unit of juvenile survival is worth slightly more than a unit of adult survival. In the batch the sensitivity to adult survival rises from `0.6536` at {math}`s_a = 0.5` to `0.7614` at {math}`0.95` while the sensitivity to juvenile survival falls from `0.9214` to `0.8254`, so the more adults survive, the more the population depends on them.

<!-- addex:end -->

## Decisions

- Use `structured::dominant` for the long-run behaviour and `demography::amplification` when a short horizon matters more than {math}`\lambda`.
- Use `elasticity` to compare entries by proportional effect and `lambda_gradient` when the entries are built from a few underlying parameters.
- Split `A` into fertilities and transitions before asking for lifetime quantities.

## Pitfalls

A projection matrix has a fixed orientation: column `j` is what a stage-`j` individual becomes. Writing it with the stages along the rows gives the transpose, which has the same growth rate but a different meaning. The program prints the dominant quantities of the page's matrix and of its transpose.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace structured = biology::population::structured;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // column j holds what a stage-j individual becomes. Writing the matrix the other way round (row j) keeps lambda but swaps the meanings
  const auto a = mat({{0.0, 0.0, 4.0}, {0.4, 0.3, 0.0}, {0.0, 0.5, 0.8}});
  for (const auto &m : {a, a.t()}) {
    const auto e = structured::dominant(m);
    out.writeln("lambda ", flat(e.lambda), ", stable mix ", flat(e.right), ", reproductive value ", flat(e.left));
  }
}
```

Output

```text
lambda [1.3574], stable mix [0.5822, 0.2202, 0.1976], reproductive value [0.3640, 1.2352, 2.6121]
lambda [1.3574], stable mix [0.0864, 0.2933, 0.6203], reproductive value [2.4518, 0.9275, 0.8320]
```

Both have `lambda = 1.3574`, so a check on the growth rate alone passes. The stable mix, however, is `(0.5822, 0.2202, 0.1976)` for the matrix and `(0.0864, 0.2933, 0.6203)` for its transpose, and the reproductive values are swapped in the same way. If a stage mix looks upside down, check the orientation of the matrix.

## Where next

[Age structure](../bio_01_growth/age_structure.md) is the continuous-age version of the same projection, and [Extinction](extinction.md) asks about the chance of dying out.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="extinction.html">
    <span class="label">Next</span>
    <span class="title">Extinction</span>
  </a>
</div>
<!-- apiref:nav:end -->
