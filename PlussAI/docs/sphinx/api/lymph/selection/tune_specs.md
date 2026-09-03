<!-- pluss-hand-authored -->

# `lymph::selection::tune_specs`

[Back to `selection`](../selection.md)

`tune_specs` is the **`ModelSpec`-grid** tuning entry point: it expands a
[`GridAxis`](GridAxis.md) space into `ModelSpec`s through a `make_spec` closure,
cross-validates each, and returns a `SpecTuneResult`. It is the spec-level
analogue of [`selection::search`](search.md) — a `make_spec` closure returning a
bare `ModelSpec` (no attached prep) instead of a `make_model` closure returning a
full `models::Model`.

## Signature

```cpp
lymph::selection::SpecTuneResult lymph::selection::tune_specs(
    const std::vector<lymph::selection::GridAxis>& axes,
    const std::function<lymph::ModelSpec(const lymph::selection::GridPoint&)>& make_spec,
    /* data */,
    const lymph::TuneOptions& tune_options = {},
    const lymph::TrainOptions& train_options = {},
    const lymph::AssessOptions& assess_options = {},
    lymph::MetricKind objective_metric = lymph::MetricKind::Accuracy,
    lymph::TuneObjectivePolicy objective_policy = lymph::TuneObjectivePolicy::Auto);
```

The `/* data */` slot is one of `const LabeledBatch&`, `(const DataFrame& x,
const Series& y)`, `(const DataFrame& x, const pluss::table::Thread& y)`, or
`(const DataFrame& frame, const DataFrameBatchOptions& data_options)`.

There is also a lower-level family that takes a pre-built `ModelSpec` list plus a
`SplitOptions` or `TuneOptions` directly (no `axes` / `make_spec`), for when the
specs are already assembled. And [`probe_specs`](#probe_specs) takes a custom
sampler for search strategies the axis front end does not wire (Bayesian,
successive halving).

## What It Does

`tune_options.search` picks the strategy:

- **`None` / `Grid`** — evaluate every point of the axis cross product.
- **`Random`** — draw `tune_options.trials` distinct points without replacement,
  seeded from `tune_options.random`. A `trials` count above `grid_size(axes)`
  falls back to the exhaustive sweep.
- **`Bayesian` / `SuccessiveHalving`** — not wired through this front end; use
  [`probe_specs`](#probe_specs).

Every visited point becomes a `ModelSpec` via `make_spec(point)`, cross-validated
under `tune_options.split`. Trials are ranked by `objective_metric`
(`objective_policy` decides the direction; `Auto` uses the metric's convention).

## `SpecTuneResult`

```cpp
struct SpecTuneResult {
  reports::Tune report{};   // trials, best_index, objective_metric/policy, objective_rank, ledger
  ModelSpec  best_spec{};   // the winning point's spec
};
```

`report.trials` is one entry per visited point — each carries its `spec`, its
`objective`, and a `CrossAssessReport`. `report.best_index` points at the winner;
`best_spec` is that winner's `ModelSpec`, ready to hand to
`lymph::models::Model` construction. `reports::Tune` is the umbrella alias for
the report type.

### Ledger accessors

When the run recorded a ledger (`has_ledger(result)` is `true`), these pull
per-trial columns out as `Series`:

| Accessor | Column |
|----------|--------|
| `objective_series(result)` | objective value per trial |
| `objective_rank_series(result)` | 1-based rank per trial (1 = best) |
| `encoded_param_table(result)` | the swept parameter values, numerically encoded |
| `tune_ledger(result)` | the whole `TuneLedger` (fit / score times, split metrics, …) |

## Parameters

`axes` — `std::vector<lymph::selection::GridAxis>`
: The search space. See [`GridAxis`](GridAxis.md).

`make_spec` — `std::function<lymph::ModelSpec(const GridPoint&)>`
: Builds one `ModelSpec` from a grid point. Read axis values with
  `point.at("axis_name")`.

`data`
: The labelled data, in any of the four shapes above.

`tune_options` — `lymph::TuneOptions`
: `{search, trials, max_parallel, split, random}` — strategy, sample size,
  workers, the CV split, and the sampler seed.

`train_options` / `assess_options`
: Per-fold training config and the metric set.

`objective_metric` / `objective_policy`
: What trials are ranked on, and the direction.

## Returns

A `lymph::selection::SpecTuneResult`.

## Errors

- An empty search space (an axis with no values) → raises.
- `SearchKind::Random` with `tune_options.trials <= 0` → raises.
- `SearchKind::Bayesian` / `SuccessiveHalving` through this front end → raises
  (use `probe_specs`).

## Practical Examples

<!-- these examples establish: the grid tune + best_spec, random sampling, and the ledger accessors. -->

### Example 1

A one-axis regularization sweep over a linear regression spec, ranked on R².

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {3., 1., 4., 1., 5., 9., 2., 6., 5., 3., 5., 8., 9., 7., 9., 3.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {2., 4., 6., 8., 10., 12., 14., 16., 18., 20., 22., 24., 26., 28., 30., 32.}, {16}, f64);
  const DataFrame frame({{"x0", Thread(x0)}, {"x1", Thread(x1)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {1e-6, 1e-3, 1e-1}},
  };
  const auto make_spec = [](const lymph::selection::GridPoint& point) {
    return lymph::ModelSpec{
        .task = lymph::TaskKind::Regression,
        .family = lymph::ModelFamily::Linear,
        .options = lymph::LinearFamilyOptions{
            .objective = lymph::LinearObjective::LeastSquares,
            .regularization = lymph::RegularizationKind::L2,
            .regularization_strength = point.at("regularization_strength"),
            .fit_intercept = true,
        },
    };
  };

  lymph::TuneOptions tune_options{};
  tune_options.search = lymph::SearchKind::Grid;
  tune_options.split.plan = lymph::CVPlanKind::CrossVal;
  tune_options.split.folds = 4;
  tune_options.split.shuffle = false;

  const lymph::selection::SpecTuneResult result =
      lymph::selection::tune_specs(axes, make_spec, frame, data_options, tune_options,
                                   lymph::TrainOptions{}, lymph::AssessOptions{},
                                   lymph::MetricKind::R2,
                                   lymph::TuneObjectivePolicy::Maximize);

  std::cout << "trials = " << result.report.trials.size()
            << ", best_index = " << result.report.best_index << "\n";
  for (const auto& trial : result.report.trials) {
    std::cout << "  strength="
              << std::get<lymph::LinearFamilyOptions>(trial.spec.options).regularization_strength
              << " objective(r2)=" << trial.objective << "\n";
  }
  std::cout << "best_spec strength = "
            << std::get<lymph::LinearFamilyOptions>(result.best_spec.options).regularization_strength
            << "\n";

  return 0;
}
```

Output

```text
trials = 3, best_index = 0
  strength=1e-06 objective(r2)=1
  strength=0.001 objective(r2)=1
  strength=0.1 objective(r2)=0.999985
best_spec strength = 1e-06
```

### Example 2

`SearchKind::Random` samples three of the eight points, seeded for
reproducibility.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {3., 1., 4., 1., 5., 9., 2., 6., 5., 3., 5., 8., 9., 7., 9., 3.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {2., 4., 6., 8., 10., 12., 14., 16., 18., 20., 22., 24., 26., 28., 30., 32.}, {16}, f64);
  const DataFrame frame({{"x0", Thread(x0)}, {"x1", Thread(x1)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {1e-6, 1e-4, 1e-2, 1.0}},
      {.name = "fit_intercept", .values = {0.0, 1.0}},
  };
  std::cout << "grid_size = " << lymph::selection::grid_size(axes) << ", sampling 3\n";

  const auto make_spec = [](const lymph::selection::GridPoint& point) {
    return lymph::ModelSpec{
        .task = lymph::TaskKind::Regression,
        .family = lymph::ModelFamily::Linear,
        .options = lymph::LinearFamilyOptions{
            .regularization_strength = point.at("regularization_strength"),
            .fit_intercept = point.at("fit_intercept") != 0.0,
        },
    };
  };

  lymph::TuneOptions tune_options{};
  tune_options.search = lymph::SearchKind::Random;
  tune_options.trials = 3;
  tune_options.random = {.seed = 20260901ULL, .deterministic = true};
  tune_options.split.plan = lymph::CVPlanKind::CrossVal;
  tune_options.split.folds = 4;
  tune_options.split.shuffle = false;

  const lymph::selection::SpecTuneResult result =
      lymph::selection::tune_specs(axes, make_spec, frame, data_options, tune_options,
                                   lymph::TrainOptions{}, lymph::AssessOptions{},
                                   lymph::MetricKind::R2,
                                   lymph::TuneObjectivePolicy::Maximize);

  std::cout << "trials run = " << result.report.trials.size() << "\n";
  for (const auto& trial : result.report.trials) {
    const auto& o = std::get<lymph::LinearFamilyOptions>(trial.spec.options);
    std::cout << "  strength=" << o.regularization_strength
              << " intercept=" << std::boolalpha << o.fit_intercept << "\n";
  }

  return 0;
}
```

Output

```text
grid_size = 8, sampling 3
trials run = 3
  strength=0.01 intercept=false
  strength=1 intercept=false
  strength=1 intercept=true
```

### Example 3

The ledger accessors — `objective_series`, `objective_rank_series`,
`encoded_param_table`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {3., 1., 4., 1., 5., 9., 2., 6., 5., 3., 5., 8., 9., 7., 9., 3.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {2., 4., 6., 8., 10., 12., 14., 16., 18., 20., 22., 24., 26., 28., 30., 32.}, {16}, f64);
  const DataFrame frame({{"x0", Thread(x0)}, {"x1", Thread(x1)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {1e-6, 1e-3, 1e-1}},
  };
  const auto make_spec = [](const lymph::selection::GridPoint& point) {
    return lymph::ModelSpec{
        .task = lymph::TaskKind::Regression,
        .family = lymph::ModelFamily::Linear,
        .options = lymph::LinearFamilyOptions{
            .regularization_strength = point.at("regularization_strength"),
            .fit_intercept = true,
        },
    };
  };

  lymph::TuneOptions tune_options{};
  tune_options.search = lymph::SearchKind::Grid;
  tune_options.split.plan = lymph::CVPlanKind::CrossVal;
  tune_options.split.folds = 4;
  tune_options.split.shuffle = false;

  const lymph::selection::SpecTuneResult result =
      lymph::selection::tune_specs(axes, make_spec, frame, data_options, tune_options,
                                   lymph::TrainOptions{}, lymph::AssessOptions{},
                                   lymph::MetricKind::R2,
                                   lymph::TuneObjectivePolicy::Maximize);

  std::cout << "has_ledger = " << std::boolalpha
            << lymph::selection::has_ledger(result) << "\n";
  const pluss::Series objective = lymph::selection::objective_series(result);
  const pluss::Series rank = lymph::selection::objective_rank_series(result);
  std::cout << "objective per trial =";
  for (const double v : objective.to_vec<double>()) std::cout << " " << v;
  std::cout << "\nrank per trial     =";
  for (const double v : rank.to_vec<double>()) std::cout << " " << v;
  std::cout << "\nencoded_param_table numel = "
            << lymph::selection::encoded_param_table(result).numel() << "\n";

  return 0;
}
```

Output

```text
has_ledger = true
objective per trial = 1 1 0.999985
rank per trial     = 1 2 3
encoded_param_table numel = 120
```

## `probe_specs`

```cpp
using ProbeSpecSampler =
    std::function</* StatusOr<ModelSpec> */(std::int64_t trial_index, std::uint64_t seed)>;

SpecTuneResult probe_specs(const ProbeSpecSampler& sampler, /* data */,
                           const SplitOptions& split = {}, /* …same trailing args… */);
SpecTuneResult probe_specs(const ProbeSpecSampler& sampler, /* data */,
                           const TuneOptions& tune_options, /* … */);
```

`probe_specs` drives the sweep from a **caller-supplied sampler** — a function
from `(trial_index, seed)` to one `ModelSpec` — instead of an axis grid. Reach
for it when the search strategy is not a plain grid or uniform random draw:
Bayesian optimization, successive halving, or any custom schedule. The return is
the same `SpecTuneResult`.

## See Also

- [`GridAxis`](GridAxis.md) — the coordinate types `axes` / `make_spec` use.
- [`selection::search`](search.md) — the full-`Model` analogue (prep included).
- [`compare`](compare.md) — rank models the caller already built, no grid.
- [`CompareReport`](CompareReport.md) — the ranked-report shape `compare` /
  `search` return (distinct from `reports::Tune`).
