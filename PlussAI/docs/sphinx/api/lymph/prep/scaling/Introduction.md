# `prep::scaling` Introduction

## What This Subfamily Owns

`prep::scaling` owns four standalone numeric scaler objects —
[`prep::StandardScaler`](StandardScaler.md), `prep::MinMaxScaler`,
`prep::RobustScaler`, `prep::MaxAbsScaler` — and the option structs that
configure them ([`options::StandardScaler`](StandardScalerOptions.md) and its
siblings).

## Object, Not Stage

Every scaler here has a twin among the [`prep::stage`](../stage.md)
factories that does identical arithmetic from the identical option struct. The
subfamily exists for the cases where you want the scaler as a *value you hold*
rather than a step folded into a model:

- **Fit once, apply many times.** `learn()` on the training frame, then `map()`
  over every fold, every batch, or the inference frame — all against the same
  parameters.
- **Read the fitted parameters.** `summary()` exposes what was learned —
  `StandardScaler` names them (`mean`, `scale`); the others expose a
  `feature_names` list and a `state` tensor vector.
- **Compose by hand.** Drop a fitted scaler into a `prep::FeatureTransformer`
  branch, or into code that is not a `lymph` model at all.

If none of those apply, the [`prep::stage`](../stage.md) factory is the
shorter path.

## The Lifecycle In One Line

Fit a standard scaler on the training frame, then map a later frame with the same fitted center and scale. The program also runs the one-hot encoder on the categorical column, to show the two stages side by side.

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

`features()` pins the columns (skip it to take every numeric column), `learn()`
fits and returns `*this` so it chains, `map()` replays. `learn_map(frame)` folds
the middle two together; `learned()` reports whether `learn` has run.

## Related Pages

- [`prep::stage`](../stage.md) — the factory twins; each stage page
  carries that transform's arithmetic and option toggles in full.
- [`prep::StandardScaler`](StandardScaler.md) — the worked-out exemplar of the
  shared method surface.
