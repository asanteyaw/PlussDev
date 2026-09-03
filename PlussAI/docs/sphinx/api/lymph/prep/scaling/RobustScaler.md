# `lymph::prep::RobustScaler`

`RobustScaler` is the standalone, fittable median/IQR scaler of the `prep` family: it centres each column on its median and divides by its interquartile range, a scaling that a few extreme values barely disturb.

## Scope And Alias

The type is `lymph::prep::RobustScaler`. It takes an [`options::RobustScaler`](RobustScalerOptions.md) (family alias `prep::RobustScalerOptions`) at construction, default-constructed if omitted. It shares its whole method surface with [`prep::StandardScaler`](StandardScaler.md) — `features` / `learn` / `map` / `learn_map` / `learned` / `summary`, with the same semantics — so that page is the reference for the mechanics. What follows is what is specific to `RobustScaler`.

## Construction

`explicit RobustScaler(RobustScalerOptions options = {});` — cheap, no data touched. The option fields are on the [`options::RobustScaler`](RobustScalerOptions.md) page; the arithmetic and its toggles are on [`prep::stage::robust_scaler`](../stage/robust_scaler.md).

## Transform

For each fitted column `map` computes `(x - median) / (q_high - q_low)`, with `q_low` / `q_high` the configured lower and upper quantiles (25th / 75th percentile by default).

## `summary()`

`RobustScaler` returns a `TransformerSummary`, not a named struct:

```cpp
struct TransformerSummary {
  std::vector<std::string> feature_names;  // the pinned / fitted columns
  std::vector<Series> state;               // raw fitted tensors, layout below
};
```

`summary().state` holds three tensors. `state[1]` is the **learned centre** (median) and `state[2]` the **learned scale** (the quantile range), one value each per fitted column; `state[0]` is internal bookkeeping. Example 2 reads them.

## Methods

The method surface is shared verbatim with [`prep::StandardScaler`](StandardScaler.md) — see that page for the full signatures, semantics, and per-method examples.

| Method | Purpose |
| --- | --- |
| [`features({...})`](StandardScaler.md#features) | pin the columns and their order (optional; without it, every numeric column) |
| [`learn(frame)`](StandardScaler.md#learn) | fit the median and quantile range per column; returns `*this` |
| [`map(frame)`](StandardScaler.md#map) | apply the fitted centre and scale to a frame; `const` |
| [`learn_map(frame)`](StandardScaler.md#learn_map) | `learn` then `map` in one call |
| [`learned()`](StandardScaler.md#learned) | has `learn` run yet? |
| [`summary()`](StandardScaler.md#summary) | fitted parameters as a `TransformerSummary` (layout above) |

## Practical Examples

### Example 1

The shared lifecycle: fit, `learn_map` one-shot, and `map` on a later frame.

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
  const pluss::Series amount = pluss::make_series({10., 20., 30., 40., 50.}, {5}, f64);
  const pluss::Series ratio  = pluss::make_series({0.2, 0.4, 0.6, 0.8, 1.0}, {5}, f64);
  const DataFrame train({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}});
  const DataFrame later({{"amount", Thread(pluss::make_series({15., 35., 60.}, {3}, f64))},
                        {"ratio",  Thread(pluss::make_series({0.3, 0.7, 1.2}, {3}, f64))}});

  lymph::prep::RobustScaler scaler;
  scaler.features({"amount", "ratio"}).learn(train);
  std::cout << "learned = " << std::boolalpha << scaler.learned() << "\n\n";
  std::cout << "learn_map(train):\n" << lymph::prep::RobustScaler{}.learn_map(train) << "\n";
  std::cout << "map(later):\n" << scaler.map(later);

  return 0;
}
```

Output

```text
learned = true

learn_map(train):
====================
     | amount  ratio
--------------------
0    | -1      -1   
1    | -0.5    -0.5 
2    |  0       0   
3    |  0.5     0.5 
4    |  1       1   
--------------------
[5 cs x 2 feat]
map(later):
====================
     | amount  ratio
--------------------
0    | -0.75   -0.75
1    |  0.25    0.25
2    |  1.5     1.5 
--------------------
[3 cs x 2 feat]
```

### Example 2

Reading the learned centre and scale out of `summary().state`.

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
  const pluss::Series amount = pluss::make_series({10., 20., 30., 40., 50.}, {5}, f64);
  const pluss::Series ratio  = pluss::make_series({0.2, 0.4, 0.6, 0.8, 1.0}, {5}, f64);
  const DataFrame train({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}});
  const DataFrame later({{"amount", Thread(pluss::make_series({15., 35., 60.}, {3}, f64))},
                        {"ratio",  Thread(pluss::make_series({0.3, 0.7, 1.2}, {3}, f64))}});

  // For RobustScaler, state[1] is the learned center (median) and state[2] the
  // learned scale (the quantile range).
  lymph::prep::RobustScaler scaler;
  scaler.features({"amount"}).learn(train);
  const auto s = scaler.summary();
  std::cout << "center (median) = " << s.state[1].to_vec<double>()[0] << "\n";
  std::cout << "scale  (IQR)    = " << s.state[2].to_vec<double>()[0] << "\n";

  return 0;
}
```

Output

```text
center (median) = 30
scale  (IQR)    = 20
```

## See Also

- [`prep::StandardScaler`](StandardScaler.md) — the shared method surface, documented in full.
- [`options::RobustScaler`](RobustScalerOptions.md) — the option fields.
- [`prep::stage::robust_scaler`](../stage/robust_scaler.md) — the factory twin; transform arithmetic and toggles.

## Changelog

- **0.1** — standalone `RobustScaler` with the shared `features` / `learn` / `map` / `learn_map` / `learned` / `summary` surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

RobustScalerOptions
```
<!-- apiref:toctree:end -->
