# `lymph::prep::Stage`

`Stage` is the attachable transformation descriptor of the `prep` family: a
transform kind plus that transform's options, with no fitted state. It is the
"what" that pairs with a [`prep::Route`](../route/Route.md) "where" and is handed
to [`Model::append`](../../models/Model/append.md),
[`Model::prepare`](../../models/Model/prepare.md), or a
`prep::FeatureTransformer` branch.

## Scope And Alias

The type is defined at `lymph::prep::stage::Stage` and re-exported one scope up as
`lymph::prep::Stage`. Both names refer to the same type; the shorter one is the
usual spelling. The `prep::stage::*` factory functions live in the inner
`lymph::prep::stage` namespace and are almost always how a `Stage` is created —
the aggregate is rarely written by hand.

```cpp
namespace lymph = pluss::ai::lymph;

// Idiomatic: a factory.
const lymph::prep::Stage a = lymph::prep::stage::standard_scaler();

// Same type, fully qualified.
const lymph::prep::stage::Stage b = lymph::prep::stage::one_hot_encoder();
```

## Construction

A `Stage` is produced by a `prep::stage::*` factory, one per transform kind. Each
factory takes that transform's option struct, default-constructed if omitted:

| Factory | Transform | Option struct |
| --- | --- | --- |
| `prep::stage::standard_scaler(opts)` | zero-mean / unit-variance scaling | `options::StandardScaler` |
| `prep::stage::minmax_scaler(opts)` | rescale to a fixed range | `options::MinMaxScaler` |
| `prep::stage::robust_scaler(opts)` | quantile-based scaling | `options::RobustScaler` |
| `prep::stage::maxabs_scaler(opts)` | divide by maximum magnitude | `options::MaxAbsScaler` |
| `prep::stage::one_hot_encoder(opts)` | categorical column → indicator columns | `options::OneHotEncoder` |
| `prep::stage::frequency_encoder(opts)` | category → observed frequency | `options::FrequencyEncoder` |
| `prep::stage::ordinal_encoder(opts)` | category → integer code | `options::OrdinalEncoder` |
| `prep::stage::label_encoder(opts)` | target label → integer code | `options::LabelEncoder` |
| `prep::stage::simple_imputer(opts)` | fill gaps with a summary statistic | `options::SimpleImputer` |
| `prep::stage::neighbor_imputer(opts)` | fill gaps from nearest rows | `options::NeighborImputer` |
| `prep::stage::iterative_imputer(opts)` | fill gaps by repeated regression | `options::IterativeImputer` |
| `prep::stage::quantile_map(opts)` | map to a uniform or Gaussian shape | `options::QuantileMap` |
| `prep::stage::power_map(opts)` | Yeo–Johnson / Box–Cox power transform | `options::PowerMap` |
| `prep::stage::threshold_binarizer(opts)` | value → 0 / 1 about a cut point | `options::ThresholdBinarizer` |
| `prep::stage::vector_normalizer(opts)` | scale each row to unit norm | `options::VectorNormalizer` |
| `prep::stage::polynomial_lift(opts)` | add polynomial / interaction columns | `options::PolynomialLift` |
| `prep::stage::custom(name, fn)` | an arbitrary `Series → Series` callable | — |

The option struct is the same one the standalone twin object takes, so
`prep::stage::standard_scaler(opts)` and `prep::StandardScaler{opts}` do identical
arithmetic (see [`prep::stage` overview](../stage_family.md)).

## State

`Stage` carries **no fitted state** — no learned means, vocabularies, or fill
values. It has two data members:

`TransformKind kind`
: Which transform this stage represents. Compare against `lymph::TransformKind`
  enumerators.

`TransformOptions options`
: A variant holding the option struct for `kind`. Set by the factory; rarely read
  directly.

Because there is no fitted state, a `Stage` value is cheap to copy and safe to
reuse — the same descriptor can be appended to several models. Fitting happens
later, inside whatever owns the plan, when a real frame is available.

## Methods

### `contract`

```cpp
lymph::prep::TransformContract lymph::prep::Stage::contract() const noexcept;
```

Returns behavioural metadata for the stage's transform kind — what the transform
*would* do to a frame — without touching data or fitting anything. It is a pure
lookup on `kind`: no allocation, never throws, and the same value for every
`Stage` of the same kind (including a default-constructed one, which reports as
`StandardScaler`). Use it to inspect a preprocessing plan rather than run it.

The returned `TransformContract` is a plain aggregate of `bool` fields:

`stateful`
: `true` if the transform learns something from training data (a mean, a
  vocabulary, a fill value) that must be replayed unchanged at inference. `false`
  for pure functions — `vector_normalizer`, `threshold_binarizer` — that behave
  identically on every frame.

`lossy`
: `true` if applying the transform discards information the original columns
  carried. Scalers are not lossy; imputers, binarizers, and frequency encoding
  are.

`reversible`
: `true` if the mapping is mathematically invertible in principle (every scaler,
  the power and quantile maps). A property of the maths, not a promise that an
  inverse method is implemented — see `inverse_available`.

`inverse_available`
: `true` only if an actual inverse operation is exposed for the kind. Currently
  `false` for every built-in kind.

`changes_feature_count`
: `true` if the transform adds or removes columns — `one_hot_encoder` (one column
  per category) and `polynomial_lift` (added interaction terms).

`changes_sample_count`
: `true` if the transform adds or removes rows. `false` for every built-in prep
  kind; reserved for row-level operations.

`target_aware`
: `true` if the transform reads the target column, not just the features.

`needs_inverse_memory`
: `true` if replaying the step at inference requires per-row state kept from fit
  time (the imputers).

`CustomStage`, returned by `prep::stage::custom(...)`, is a distinct type with its
own `name`, `map_fn`, and `contract` members rather than a `Stage` with a special
kind.

#### Example 1

The full field set across five kinds. Scalers are stateful but lossless and
shape-preserving; `one_hot_encoder` widens the frame; `polynomial_lift` and
`threshold_binarizer` are stateless.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <utility>

int main() {
  namespace lymph = pluss::ai::lymph;

  // contract() answers questions about a stage kind without running it: the
  // metadata you need to audit a preprocessing plan rather than execute it.
  const std::pair<const char*, lymph::prep::Stage> stages[] = {
      {"standard_scaler", lymph::prep::stage::standard_scaler()},
      {"one_hot_encoder", lymph::prep::stage::one_hot_encoder()},
      {"simple_imputer", lymph::prep::stage::simple_imputer()},
      {"polynomial_lift", lymph::prep::stage::polynomial_lift()},
      {"threshold_binarizer", lymph::prep::stage::threshold_binarizer()},
  };

  for (const auto& [label, stage] : stages) {
    const lymph::prep::TransformContract c = stage.contract();
    std::cout << label
              << ": stateful=" << std::boolalpha << c.stateful
              << " lossy=" << c.lossy
              << " reversible=" << c.reversible
              << " changes_feature_count=" << c.changes_feature_count
              << " changes_sample_count=" << c.changes_sample_count << "\n";
  }

  return 0;
}
```

Output

```text
standard_scaler: stateful=true lossy=false reversible=true changes_feature_count=false changes_sample_count=false
one_hot_encoder: stateful=true lossy=false reversible=true changes_feature_count=true changes_sample_count=false
simple_imputer: stateful=true lossy=true reversible=false changes_feature_count=false changes_sample_count=false
polynomial_lift: stateful=false lossy=true reversible=false changes_feature_count=true changes_sample_count=false
threshold_binarizer: stateful=false lossy=true reversible=false changes_feature_count=false changes_sample_count=false
```

#### Example 2

Using one field to answer a question about a whole plan: how many steps will
change the width of the frame before it reaches the estimator.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <utility>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  // A plan under review: several stages, and a question -- how many of them
  // will change the width of the frame before it reaches the estimator?
  const std::vector<std::pair<const char*, lymph::prep::Stage>> plan = {
      {"scale amount", lymph::prep::stage::standard_scaler()},
      {"encode region", lymph::prep::stage::one_hot_encoder()},
      {"lift interactions", lymph::prep::stage::polynomial_lift()},
      {"scale ratio", lymph::prep::stage::minmax_scaler()},
  };

  int widening = 0;
  for (const auto& [note, stage] : plan) {
    const bool changes_width = stage.contract().changes_feature_count;
    std::cout << (changes_width ? "[width +] " : "[width =] ") << note << "\n";
    if (changes_width) ++widening;
  }
  std::cout << "\nstages that change feature count: " << widening
            << " of " << plan.size() << "\n";

  return 0;
}
```

Output

```text
[width =] scale amount
[width +] encode region
[width +] lift interactions
[width =] scale ratio

stages that change feature count: 2 of 4
```

#### Example 3

`stateful` separates stages that must be fitted on training data from pure
functions. `needs_inverse_memory` marks the ones that also keep per-row state.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <utility>

int main() {
  namespace lymph = pluss::ai::lymph;

  // stateful tells you whether a stage must be *fitted* on training data
  // (and its learned state replayed at inference) or is a pure function that
  // behaves identically on every frame.
  const std::pair<const char*, lymph::prep::Stage> stages[] = {
      {"standard_scaler", lymph::prep::stage::standard_scaler()},
      {"quantile_map", lymph::prep::stage::quantile_map()},
      {"vector_normalizer", lymph::prep::stage::vector_normalizer()},
      {"threshold_binarizer", lymph::prep::stage::threshold_binarizer()},
  };

  for (const auto& [label, stage] : stages) {
    const lymph::prep::TransformContract c = stage.contract();
    std::cout << label << ": "
              << (c.stateful ? "fitted on training data"
                             : "pure function, no fitting")
              << (c.needs_inverse_memory ? " (keeps inverse memory)" : "")
              << "\n";
  }

  return 0;
}
```

Output

```text
standard_scaler: fitted on training data
quantile_map: fitted on training data
vector_normalizer: pure function, no fitting
threshold_binarizer: pure function, no fitting
```

## Notes

**Descriptor vs. standalone.** Do not confuse a stage factory with a standalone
transformer object. `prep::stage::standard_scaler(...)` returns a `Stage`;
`prep::StandardScaler(...)` returns a reusable fitted object. They share option
types but serve different roles — see the
[`prep::stage` overview](../stage_family.md).

**Pairing.** `Stage` becomes meaningful only when paired with a `Route`. On its
own it says what kind of transform exists, not which features it should touch.

## Changelog

- **0.1** — initial `Stage` descriptor, `prep::stage::*` factory set, and
  `contract()`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
