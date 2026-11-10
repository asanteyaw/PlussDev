<!-- generated from Lsr1Options.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::Lsr1Options`

The L-SR1 method's own settings: memory, trust-region radius and its update rules, the SR1 skip test and the line search.

`memory_size` is the number of step pairs kept; too few and the model cannot follow a curved valley. `initial_radius` and `max_radius` set the trust-region size, which the method adapts: when the ratio of actual to predicted decrease exceeds `eta_3` the radius grows by `eta_4`; when it is below `eta_1` the radius shrinks. `initial_gamma` and `gamma_shrink` control the scalar `γ` of the initial model `γI`, kept strictly below the smallest eigenvalue estimate by the factor `gamma_shrink` (the paper names a `γ` that is too large as the most common cause of failure). `sr1_skip_tolerance` is the classical skip test that rejects a rank-one update when its denominator is too small. A backtracking line search with `wolfe_c1`, `line_search_shrink` and `max_line_search_iterations` guards each step. `newton_tolerance`, `max_newton_iterations` and `zero_tolerance` control the numerics of the trust-region subproblem solve.

## Fields

### `memory_size`

`std::int64_t`, default `10`.

Step pairs kept by the model.

### `initial_radius`

`double`, default `1.0`.

Starting trust-region radius.

### `max_radius`

`double`, default `1e8`.

Largest radius; an `initial_radius` above it starts at the cap.

### `initial_gamma`

`double`, default `1.0`.

Scalar of the initial model `γI` before any pair exists.

### `gamma_shrink`

`double`, default `0.9`.

Factor keeping `γ` strictly below the smallest eigenvalue estimate; in (0, 1).

### `sr1_skip_tolerance`

`double`, default `1e-8`.

An update whose denominator is below this fraction of its norms is skipped.

### `tau_2`

`double`, default `0.1`.

Radius-update constant of the algorithm.

### `tau_3`

`double`, default `0.75`.

Radius-update constant of the algorithm.

### `eta_1`

`double`, default `0.25`.

Ratio below which the radius shrinks.

### `eta_2`

`double`, default `0.5`.

Intermediate acceptance ratio.

### `eta_3`

`double`, default `0.75`.

Ratio above which the radius may grow.

### `eta_4`

`double`, default `2.0`.

Growth factor of the radius.

### `wolfe_c1`

`double`, default `1e-4`.

Sufficient-decrease constant of the line search.

### `max_line_search_iterations`

`std::int64_t`, default `20`.

The most step reductions.

### `line_search_shrink`

`double`, default `0.5`.

Factor applied per reduction.

### `newton_tolerance`

`double`, default `1e-10`.

Tolerance of the trust-region subproblem solve.

### `max_newton_iterations`

`std::int64_t`, default `100`.

Iteration cap of that solve.

### `zero_tolerance`

`double`, default `1e-10`.

Threshold for treating a quantity as zero.

## Guidance

Set as `Lsr1SolveOptions::step`.

## Practical Examples

### Example 1

Memory on the chained Rosenbrock function. A longer memory follows the valley better.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> head_index, tail_index, start;
  for (int i = 0; i < 9; ++i) { head_index.push_back(i); tail_index.push_back(i + 1); }
  for (int i = 0; i < 10; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const Series head = pluss::make_series(head_index, {9}).to(pluss::ScalarType::Int64);
  const Series tail = pluss::make_series(tail_index, {9}).to(pluss::ScalarType::Int64);
  const auto chain = [&](const Series &x) {
    const Series a = x.index_select(0, head), b = x.index_select(0, tail);
    return (100.0 * (b - a.square()).square() + (1.0 - a).square()).sum();
  };

  for (const std::int64_t memory : {1, 3, 10, 30}) {
    optim::Lsr1SolveOptions options;
    options.max_iterations = 1000;
    options.step.memory_size = memory;
    const optim::Lsr1Result result = optim::l_sr1_trust_region(chain, pluss::make_series(start, {10}), options);
    out.writeln("memory", memory, ": exit =", optim::lsr1_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
  }
}
```

Output

```text
memory 1 : exit = MaxIterations , iterations = 1000 , evaluations = 1256
memory 3 : exit = MaxIterations , iterations = 1000 , evaluations = 1538
memory 10 : exit = Converged , iterations = 115 , evaluations = 262
memory 30 : exit = Converged , iterations = 115 , evaluations = 262
```

### Example 2

The starting trust radius, which the method then adapts. A radius that is far too small costs iterations while it grows, but the run is not cut short.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> head_index, tail_index, start;
  for (int i = 0; i < 9; ++i) { head_index.push_back(i); tail_index.push_back(i + 1); }
  for (int i = 0; i < 10; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const Series head = pluss::make_series(head_index, {9}).to(pluss::ScalarType::Int64);
  const Series tail = pluss::make_series(tail_index, {9}).to(pluss::ScalarType::Int64);
  const auto chain = [&](const Series &x) {
    const Series a = x.index_select(0, head), b = x.index_select(0, tail);
    return (100.0 * (b - a.square()).square() + (1.0 - a).square()).sum();
  };

  for (const double radius : {0.01, 1.0, 100.0}) {
    optim::Lsr1SolveOptions options;
    options.max_iterations = 1000;
    options.step.initial_radius = radius;
    const optim::Lsr1Result result = optim::l_sr1_trust_region(chain, pluss::make_series(start, {10}), options);
    out.writeln("initial radius", radius, ": exit =", optim::lsr1_exit_flag_name(result.exit_flag), ", iterations =", result.iteration);
  }
}
```

Output

```text
initial radius 0.0100 : exit = Converged , iterations = 148
initial radius 1.0000 : exit = Converged , iterations = 115
initial radius 100.0000 : exit = Converged , iterations = 145
```

### Example 3

Capping the radius. An `initial_radius` above `max_radius` is allowed: the run starts at the cap.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> head_index, tail_index, start;
  for (int i = 0; i < 9; ++i) { head_index.push_back(i); tail_index.push_back(i + 1); }
  for (int i = 0; i < 10; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const Series head = pluss::make_series(head_index, {9}).to(pluss::ScalarType::Int64);
  const Series tail = pluss::make_series(tail_index, {9}).to(pluss::ScalarType::Int64);
  const auto chain = [&](const Series &x) {
    const Series a = x.index_select(0, head), b = x.index_select(0, tail);
    return (100.0 * (b - a.square()).square() + (1.0 - a).square()).sum();
  };

  optim::Lsr1SolveOptions options;
  options.max_iterations = 1000;
  options.step.initial_radius = 5.0;
  options.step.max_radius = 0.5;
  const optim::Lsr1Result result = optim::l_sr1_trust_region(chain, pluss::make_series(start, {10}), options);
  out.writeln("exit =", optim::lsr1_exit_flag_name(result.exit_flag), ", iterations =", result.iteration);
}
```

Output

```text
exit = Converged , iterations = 150
```

## See Also

- [`Lsr1SolveOptions`](Lsr1SolveOptions.md): holds it
- [`l_sr1_trust_region`](l_sr1_trust_region.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Lsr1ExitFlag.html">
    <span class="label">Previous</span>
    <span class="title">Lsr1ExitFlag</span>
  </a>
  <a class="next-link" href="Lsr1ProgressSnapshot.html">
    <span class="label">Next</span>
    <span class="title">Lsr1ProgressSnapshot</span>
  </a>
</div>
<!-- apiref:nav:end -->
