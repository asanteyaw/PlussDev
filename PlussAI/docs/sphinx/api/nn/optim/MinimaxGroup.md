# `pluss::ai::nn::optim::MinimaxGroup`

One per-parameter-group entry for [`Minimax`](Minimax.md)'s per-group
constructor: a parameter subset paired with its own
[`MinimaxOptions`](MinimaxOptions.md). Unlike the other six optimizers'
own `Group` types, constraints are never part of a `MinimaxGroup` --
[`MinimaxConstraints`](MinimaxConstraints.md) is supplied once, separately,
shared across every group in the same `Minimax` instance.

## Signature

```cpp
std::vector<pluss::Series*> parameters;
nn::optim::MinimaxOptions options;

nn::optim::MinimaxGroup::MinimaxGroup();
nn::optim::MinimaxGroup::MinimaxGroup(pluss::RangeView<pluss::Series*> parameters, nn::optim::MinimaxOptions options = {});
nn::optim::MinimaxGroup::MinimaxGroup(std::initializer_list<pluss::Series*> parameters, nn::optim::MinimaxOptions options = {});
```

Unlike every other `*Group` type in this reference (plain aggregates),
`MinimaxGroup` has explicit constructors -- the `initializer_list`
overload exists specifically so a bare `{&a, &b}` parameter list works
directly at a call site (see Example 1), the same disambiguation
[`Schedule::append`](../module/Schedule/append.md)'s own
`initializer_list` overload provides.

## Construction

```cpp
nn::optim::MinimaxGroup{{&backbone_weight}, nn::optim::MinimaxOptions{.primal_method = optim::MinimaxPrimalMethod::AdamW}};
nn::optim::MinimaxGroup{{&head_weight}, nn::optim::MinimaxOptions{.primal_method = optim::MinimaxPrimalMethod::SGD}};
```

A `vector<MinimaxGroup>` (or `initializer_list<MinimaxGroup>`) of these is
passed to [`Minimax`](Minimax.md)'s per-group constructor overload,
alongside one shared [`MinimaxConstraints`](MinimaxConstraints.md). Every
group's own `MinimaxOptions` may choose a different `primal_method` and
its own hyperparameters, but every group must share identical values for
every constraint-control field (`dual_learning_rate` through
`schedule_max_scale`, see [`MinimaxOptions`](MinimaxOptions.md#guidance))
-- construction throws otherwise. Every parameter across every group must
be unique.

## State

`parameters` -- `vector<Series*>`
: This group's own parameter subset.

`options` -- [`MinimaxOptions`](MinimaxOptions.md)
: This group's own primal-method choice and hyperparameters, independent
  of every other group's, including its own moment/momentum buffers per
  parameter.

## Methods

None -- `MinimaxGroup` is a plain data carrier, consumed only by
[`Minimax`](Minimax.md)'s constructor.

## Practical Examples

One example: two groups under one shared inequality constraint, each
driven by a different primal method.

### Example 1

Both groups target the same constraint (`x <= 3`) and the same
constraint-control fields, but one group's primal step is driven by
`Adam`, the other by `SGD` -- different trajectories toward the same
constrained region.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>
#include <pluss/subsp/math/diff.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  namespace optim = nn::optim;
  namespace diff = pluss::subsp::math::diff;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series target = pluss::make_series(5.0F, f32);

  optim::MinimaxOptions adam_opts{};
  adam_opts.primal_method = optim::MinimaxPrimalMethod::Adam;
  adam_opts.learning_rate = 0.1;
  adam_opts.dual_learning_rate = 0.2;
  adam_opts.inequality_penalty = 10.0;

  optim::MinimaxOptions sgd_opts = adam_opts;
  sgd_opts.primal_method = optim::MinimaxPrimalMethod::SGD;
  sgd_opts.learning_rate = 0.2;

  optim::MinimaxConstraints constraints{};
  constraints.inequalities = [](const std::vector<Series>& vars) {
    return std::vector<Series>{vars[0].sub(3.0), vars[1].sub(3.0)};
  };
  constraints.inequalities_trace = [](const std::vector<diff::Trace>& vars) {
    return std::vector<diff::Trace>{vars[0] - 3.0, vars[1] - 3.0};
  };

  Series x_adam = pluss::make_series(0.0F, f32);
  Series x_sgd = pluss::make_series(0.0F, f32);

  std::vector<optim::MinimaxGroup> groups{
      optim::MinimaxGroup{{&x_adam}, adam_opts},
      optim::MinimaxGroup{{&x_sgd}, sgd_opts},
  };
  optim::Minimax minimax(groups, constraints);

  for (int step = 0; step < 60; ++step) {
    minimax.step(std::vector<Series>{x_adam.sub(target), x_sgd.sub(target)});
  }
  std::cout << "Adam-driven group: x = " << x_adam.item<float>() << "\n";
  std::cout << "SGD-driven group:  x = " << x_sgd.item<float>() << "\n";

  return 0;
}
```

Output

```text
Adam-driven group: x = 2.97
SGD-driven group:  x = 2.7866
```

## Changelog

- **0.1** — a parameter list paired with its own `MinimaxOptions`, with
  constraints supplied separately and shared across every group.

## See Also

- [`Minimax`](Minimax.md) -- the optimizer this configures per group.
- [`MinimaxOptions`](MinimaxOptions.md) -- the fields each group's own
  `options` carries.
- [`MinimaxConstraints`](MinimaxConstraints.md) -- shared across every
  group, never part of `MinimaxGroup` itself.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
