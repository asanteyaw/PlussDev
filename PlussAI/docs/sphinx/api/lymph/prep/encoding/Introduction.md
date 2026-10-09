# `prep::encoding` Introduction

## What This Subfamily Owns

`prep::encoding` owns six standalone categorical-encoder objects —
[`prep::OneHotEncoder`](OneHotEncoder.md),
[`prep::OrdinalEncoder`](OrdinalEncoder.md),
[`prep::LabelEncoder`](LabelEncoder.md),
[`prep::FrequencyEncoder`](FrequencyEncoder.md),
[`prep::BaseNEncoder`](BaseNEncoder.md),
[`prep::HashingEncoder`](HashingEncoder.md) — and the option structs that
configure them ([`options::OneHotEncoder`](OneHotEncoderOptions.md) and its
siblings).

## Object, Not Stage

Every encoder here has a twin among the [`prep::stage`](../stage.md)
factories that does identical arithmetic from the identical option struct. The
subfamily exists for the cases where you want the encoder as a *value you hold*
rather than a step folded into a model:

- **Fit once, apply many times.** `learn()` on the training frame, then `map()`
  over every fold, every batch, or the inference frame — all against the same
  category layout.
- **Read the learned layout.** `summary()` gives the generic
  `TransformerSummary`; `OneHotEncoder` / `OrdinalEncoder` / `LabelEncoder` /
  `FrequencyEncoder` additionally expose `categories_` (the learned category set
  per feature) and `mapped_names_` (the output column labels).
- **Compose by hand.** Drop a fitted encoder into a `prep::FeatureTransformer`
  branch, or into code that is not a `lymph` model at all.

The supervised encoders — target and weight-of-evidence — need the label at fit
time and so have no standalone object form; use
[`prep::stage::target_encoder`](../stage/target_encoder.md) and
[`prep::stage::woe_encoder`](../stage/woe_encoder.md).

## The Lifecycle In One Line

Fit a one-hot encoder on the training frame, then map a later frame with it. The program also fits a standard scaler on the numeric columns of the same frames. It checks that the scaled column survives and the encoded column is replaced.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({
      {"amount", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4}, cfg))},
      {"ratio", Slate(pluss::make_series(std::vector<double>{0.1, 0.2, 0.3, 0.4}, {4}, cfg))},
      {"region", Slate(pluss::make_note({"north", "south", "north", "east"}))},
  });
  const DataFrame later({
      {"amount", Slate(pluss::make_series(std::vector<double>{25.0}, {1}, cfg))},
      {"ratio", Slate(pluss::make_series(std::vector<double>{0.25}, {1}, cfg))},
      {"region", Slate(pluss::make_note({"south"}))},
  });

  lymph::prep::StandardScaler scaler;
  const DataFrame scaled = scaler.features({"amount", "ratio"}).learn(train).map(later);
  std::cout << "amount kept = " << (scaled.get("amount").has_value() ? "true" : "false") << "\n";

  lymph::prep::OneHotEncoder encoder;
  const DataFrame wide = encoder.features({"region"}).learn(train).map(later);
  std::cout << "region replaced by encoded columns = " << (wide.get("region").has_value() ? "false" : "true") << "\n";
  return 0;
}
```

Output

```text
amount kept = true
region replaced by encoded columns = true
```

`features()` pins the categorical columns (**required** on the `DataFrame`
path), `learn()` records each column's category set and returns `*this` so it
chains, `map()` replays. `learn_map(frame)` folds the middle two together;
`learned()` reports whether `learn` has run.

## Unknowns Are A Two-Part Decision

Reserving a bucket for categories unseen at fit time is a fit-time option
(`include_unknown_bucket`). What `map` does when it *meets* an unknown is a
replay-time choice on [`InferOptions::unknown_categorical`](OneHotEncoder.md#map)
— `Error` (the default), `MissingBucket`, or `Ignore`.

## Related Pages

- [`prep::stage`](../stage.md) — the factory twins; each stage page
  carries that transform's arithmetic and option toggles in full.
- [`prep::OneHotEncoder`](OneHotEncoder.md) — the worked-out exemplar of the
  shared method surface.
