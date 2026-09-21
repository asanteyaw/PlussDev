# Numerical Workflows

This guide introduces Pluss numeric work with `Series` and points to the deeper
math, statistics, optimization, autodiff, and finance guides.

## Includes

For core numeric work:

```cpp
#include <pluss/pluss.h>
```

For PlussMa math and statistics surfaces:

```cpp
#include <pluss/ma.h>
```

## Create Series values

```cpp
auto scalar = pluss::make_series(3.14);
auto vector = pluss::make_series({1.0, 2.0, 3.0});
auto matrix = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
});

std::vector<double> flat{1, 2, 3, 4, 5, 6};
auto shaped = pluss::make_series(flat, {2, 3});
```

Factory helpers:

```cpp
auto zeros = pluss::series::zeros({2, 3});
auto ones = pluss::series::ones({2, 3});
auto full = pluss::series::full({2, 3}, 7.5);
auto grid = pluss::series::linspace(0.0, 1.0, 11);
auto noise = pluss::series::randn({1000});
```

## Inspect and extract values

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});

double mean = x.mean().item<double>();
std::vector<double> values = x.to_vec<double>();

auto meta = x.attr();
std::cout << meta << '\n';
```

Use `item<T>()` only for scalar `Series` values. Use `to_vec<T>()` or host
extraction helpers for vectors and matrices.

## Elementwise math

```cpp
auto x = pluss::series::linspace(-1.0, 1.0, 5);
auto y = x.pow(2.0) + 2.0 * x + 1.0;

auto positive = y.gt(0.0);
auto clipped = y.clamp_min(0.0);
auto normalized = (y - y.mean()) / y.std(false);
```

Method and operator styles are both used in the codebase. Method style is
useful when a name would otherwise be ambiguous:

```cpp
auto z = x.add(1.0).mul(2.0).sqrt();
```

## Shape operations

```cpp
auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});

auto flat = x.flatten(0, 1);
auto reshaped = x.reshape({3, 2});
auto transposed = x.transpose(0, 1);
auto row = x.select(0, 1);
```

See [Series Guide](series.md) for extensive shape, indexing, masking, gather,
scatter, and reduction examples.

## Reductions and statistics

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});

auto total = x.sum();
auto mean = x.mean();
auto variance = x.var(false);
auto standard_deviation = x.std(false);
auto max_value = x.max();
```

Axis reductions are available on higher-rank series:

```cpp
auto matrix = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

auto row_means = matrix.mean(1, false);
auto column_sums = matrix.sum(0, false);
```

## Distributions

PlussMa distribution objects operate on `Series`:

```cpp
#include <pluss/ma.h>

pluss::subsp::stats::distr::Normal normal{0.0, 1.0};
auto xs = pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0});

auto pdf = normal.pdf(xs);
auto logpdf = normal.logpdf(xs);
auto cdf = normal.cdf(xs);
```

See:

- [Distributions User Guide](plussma_distributions_user_guide.md)
- [Distributions Cookbook](plussma_distributions_cookbook.md)

## Optimization

Optimization APIs use `Series` for parameters, objectives, bounds, and results:

```cpp
#include <pluss/ma.h>

using namespace pluss::subsp::math::optimization;

Problem problem;
problem.initial = pluss::make_series(std::vector<double>{3.0, -2.0}, {2});
problem.objective = [](const pluss::Series& x) {
  return x.pow(2.0).sum();
};

auto result = minimize(problem);
```

See [Optimization User Guide](plussma_optimization_user_guide.md) and
[Optimization Cookbook](plussma_optimization_cookbook.md) for the concrete
solver families and option structs available in your build.

## Differentiation and autodiff

Core differentiable workflows use `Series` and the math diff layer:

```cpp
auto x = pluss::make_series(1.25).to(pluss::codem::ScalarType::Float64);

auto f = [](const pluss::Series& v) {
  return v.pow(3.0) - v;
};

auto grad = pluss::subsp::math::diff::grad(f, x);
```

See:

- [Autodiff Guide](autodiff.md)
- [Autodiff API Reference](autodiff_api_reference.md)
- [Autodiff Architecture](autodiff_architecture.md)

## Torch interop

When Pluss is built with `PLUSS_ENABLE_TORCH=ON`, the SDK exports Torch interop
helpers through `<pluss/pluss.h>` / `<pluss/core.h>`:

```cpp
auto pluss_series = pluss::series::from_torch(tensor);
auto torch_tensor = pluss_series.tensor();
```

Use a Torch-enabled SDK and keep Torch visible to your downstream CMake project
when your application also uses `torch::Tensor` directly.

## Serialization

Use unified save/load for core numeric artifacts:

```cpp
auto weights = pluss::series::randn({16, 8});
pluss::save(weights, "weights.plio");

auto loaded = pluss::load<pluss::Series>("weights.plio");
```

See [I/O Save/Load Guide](io_save_load_guide.md).

## Debugging numeric code

Use these checks during development:

```cpp
std::cout << x.attr() << '\n';
std::cout << x.mean().item<double>() << '\n';
std::cout << x.isfinite().all().item<bool>() << '\n';
```

For table-backed numeric work, inspect the inferred columns before doing math:

```cpp
df.info();
df.series_features().describe();
```

## Related pages

- [Series Guide](series.md)
- [Series API Reference](series_api_reference.md)
- [Series All Methods Usage](series_all_methods_usage.md)
- [Regression User Guide](plussma_regression_user_guide.md)
- [Finance User Guide](plussma_finance_user_guide.md)
- [Hypothesis User Guide](plussma_hypothesis_user_guide.md)
