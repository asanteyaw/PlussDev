# Lymph `options`

`lymph::options` is the **umbrella alias namespace** for option structs —
one short spelling per struct, regardless of which family actually owns and
defines it. `options::DFBatch` and `DataFrameBatchOptions` are the same type;
use whichever reads better in context. Cross-family code (a `Search::run`
signature that needs a batch shape, a validation shape, and an assess shape
all at once) tends to read cleaner with the short spelling; a family's own
pages use its native name.

`lymph::options` holds no behavior of its own — it is exactly this list of
`using` aliases, defined in `lymph/options.h`.

## Data And Batch Shapes

| `options::` alias | Underlying type | Home |
| --- | --- | --- |
| `options::DFBatch` | `DataFrameBatchOptions` | `lymph::` (column names + label feature for a `DataFrame`) |
| `options::Infer` | `InferOptions` | `lymph::` (`positive_threshold` and friends) |
| `options::Split` | `SplitOptions` | `lymph::` (the low-level split policy `CrossValOptions` compiles down to) |
| `options::HoldoutSplit` | `prep::HoldoutSplitOptions` | [`prep`](prep.md) |
| `options::TimeSplit` | `prep::TimeSeriesSplitOptions` | [`prep`](prep.md) |

## Validation, Tuning, Assessment

| `options::` alias | Underlying type | Home |
| --- | --- | --- |
| `options::CrossVal` | `selection::CrossValOptions` | [`selection::validate`](selection/validate/CrossValOptions.md) |
| `options::Fit` | `selection::FitOptions` | [`selection`](selection.md) (`fit_top` / `fit_bottom` re-fit policy) |
| `options::Tune` | `TuneOptions` | [`selection::tune_specs`](selection/tune_specs.md) |
| `options::Train` | `TrainOptions` | `lymph::` (per-fold training configuration every `learn(...)` takes) |
| `options::Assess` | `AssessOptions` | `lymph::` (which metrics `assess(...)` / `run(...)` compute) |
| `options::ClassReport` | `ClassReportOptions` | [`metrics::classification`](metrics/classification_summary.md) |
| `options::Confusion` | `ConfusionOptions` | [`metrics::classification`](metrics/confusion_table.md) |

## Workflow And Shape

| `options::` alias | Underlying type | Home |
| --- | --- | --- |
| `options::Workflow` | `workflow::WorkflowOptions` | [`workflow`](workflow.md) |
| `options::Time` | `workflow::TimeOptions` | [`workflow`](workflow.md) (time-aware pipeline settings) |
| `options::Profile` | `shape::ProfileOptions` | `lymph::shape` (feature distribution profiling) |

## Model-Family Options

Every trainable family's construction options carry a short `options::`
spelling — pass either spelling to that family's factory function
(`models::linear_regression(options::Linear{...})` and
`models::linear_regression(LinearFamilyOptions{...})` build the same model).

| `options::` alias | Underlying type |
| --- | --- |
| `options::Linear` | `LinearFamilyOptions` |
| `options::Tree` | `TreeFamilyOptions` |
| `options::Ensemble` | `EnsembleFamilyOptions` |
| `options::Neighbor` | `NeighborFamilyOptions` |
| `options::Bayes` | `BayesFamilyOptions` |
| `options::Kernel` | `KernelFamilyOptions` |
| `options::Cluster` | `ClusterFamilyOptions` |
| `options::Decompose` | `DecomposeFamilyOptions` |
| `options::Detect` | `DetectFamilyOptions` |
| `options::Transform` | `TransformOptions` |

## Prep Stage Options

Every `prep::stage::*` factory takes its own `*Options` struct; `options::`
gives each one a name without the family qualifier — most are documented on
their own page today, listed below where that page exists.

| `options::` alias | Underlying type |
| --- | --- |
| `options::StandardScaler` | [`prep::StandardScalerOptions`](prep/scaling/StandardScalerOptions.md) |
| `options::MinMaxScaler` | [`prep::MinMaxScalerOptions`](prep/scaling/MinMaxScalerOptions.md) |
| `options::MaxAbsScaler` | [`prep::MaxAbsScalerOptions`](prep/scaling/MaxAbsScalerOptions.md) |
| `options::RobustScaler` | [`prep::RobustScalerOptions`](prep/scaling/RobustScalerOptions.md) |
| `options::OneHotEncoder` | [`prep::OneHotEncoderOptions`](prep/encoding/OneHotEncoderOptions.md) |
| `options::OrdinalEncoder` | [`prep::OrdinalEncoderOptions`](prep/encoding/OrdinalEncoderOptions.md) |
| `options::LabelEncoder` | [`prep::LabelEncoderOptions`](prep/encoding/LabelEncoderOptions.md) |
| `options::BaseNEncoder` | [`prep::BaseNEncoderOptions`](prep/encoding/BaseNEncoderOptions.md) |
| `options::HashingEncoder` | [`prep::HashingEncoderOptions`](prep/encoding/HashingEncoderOptions.md) |
| `options::FrequencyEncoder` | [`prep::FrequencyEncoderOptions`](prep/encoding/FrequencyEncoderOptions.md) |
| `options::CategoryIndexer` | `prep::CategoryIndexerOptions` |
| `options::TargetEncoder` | `prep::TargetEncoderOptions` |
| `options::WeightOfEvidence` | `prep::WOEEncoderOptions` |
| `options::SimpleImputer` | `prep::SimpleImputerOptions` |
| `options::IterativeImputer` | `prep::IterativeImputerOptions` |
| `options::NeighborImputer` | `prep::NeighborImputerOptions` |
| `options::OutlierIsolator` | `prep::OutlierIsolatorOptions` |
| `options::FunctionTransformer` | `prep::FunctionTransformerOptions` |
| `options::PolynomialLift` | `prep::PolynomialLiftOptions` |
| `options::PowerMap` | `prep::PowerMapOptions` |
| `options::QuantileMap` | `prep::QuantileMapOptions` |
| `options::ThresholdBinarizer` | `prep::ThresholdBinarizerOptions` |
| `options::VectorNormalizer` | `prep::VectorNormalizerOptions` |

## Related Pages

- [`prep`](prep.md) — most encoder / scaler / imputer options live here.
- [`models`](models.md) — the family options these factories take.
- [`selection`](selection.md) / [`workflow`](workflow.md) — `CrossVal`,
  `Fit`, `Tune`, `Workflow`, `Time`.
- [`reports`](reports.md) — the umbrella pattern this mirrors, for report
  *types* rather than option structs.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

```
<!-- apiref:toctree:end -->
