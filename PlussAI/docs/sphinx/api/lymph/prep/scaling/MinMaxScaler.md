# `lymph::prep::MinMaxScaler`

`MinMaxScaler` is the standalone, fittable range scaler of the `prep` family: it learns each column's minimum and maximum and rescales it onto a fixed `[lower, upper]` range (default `[0, 1]`).

## Scope And Alias

The type is `lymph::prep::MinMaxScaler`. It takes an [`options::MinMaxScaler`](MinMaxScalerOptions.md) (family alias `prep::MinMaxScalerOptions`) at construction, default-constructed if omitted. It shares its whole method surface with [`prep::StandardScaler`](StandardScaler.md) — `features` / `learn` / `map` / `learn_map` / `learned` / `summary`, with the same semantics — so that page is the reference for the mechanics. What follows is what is specific to `MinMaxScaler`.

## Construction

`explicit MinMaxScaler(MinMaxScalerOptions options = {});` — cheap, no data touched. The option fields are on the [`options::MinMaxScaler`](MinMaxScalerOptions.md) page; the arithmetic and its toggles are on [`prep::stage::minmax_scaler`](../stage/minmax_scaler.md).

## Transform

For each fitted column `map` computes `lower + (x - min) / (max - min) * (upper - lower)`. A value outside the learned `[min, max]` maps outside the target range unless `clip` is set.

## `summary()`

`MinMaxScaler` returns a `TransformerSummary`, not a named struct:

```cpp
struct TransformerSummary {
  std::vector<std::string> feature_names;  // the pinned / fitted columns
  std::vector<Series> state;               // raw fitted tensors, layout below
};
```

`summary().state` holds three tensors. `state[1]` is the **learned minimum** and `state[2]` the **learned maximum**, one value each per fitted column; `state[0]` is internal bookkeeping. Example 2 reads them.

## Methods

The method surface is shared verbatim with [`prep::StandardScaler`](StandardScaler.md) — see that page for the full signatures, semantics, and per-method examples.

| Method | Purpose |
| --- | --- |
| [`features({...})`](StandardScaler.md#features) | pin the columns and their order (optional; without it, every numeric column) |
| [`learn(frame)`](StandardScaler.md#learn) | fit `min` / `max` per column; returns `*this` |
| [`map(frame)`](StandardScaler.md#map) | apply the fitted range to a frame; `const` |
| [`learn_map(frame)`](StandardScaler.md#learn_map) | `learn` then `map` in one call |
| [`learned()`](StandardScaler.md#learned) | has `learn` run yet? |
| [`summary()`](StandardScaler.md#summary) | fitted parameters as a `TransformerSummary` (layout above) |

## Practical Examples

### Example 1

The shared lifecycle: `features().learn()` chains, `map()` replays, out-of-range rows land outside `[0, 1]`.

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

  // MinMaxScaler shares the object lifecycle: features(), learn() (chains),
  // map(), learn_map(), learned(), summary().
  lymph::prep::MinMaxScaler scaler;
  scaler.features({"amount", "ratio"}).learn(train);
  std::cout << "learned = " << std::boolalpha << scaler.learned() << "\n\n";
  std::cout << "map(train):\n" << scaler.map(train) << "\n";
  std::cout << "map(later) -- out-of-range rows go outside [0, 1]:\n" << scaler.map(later);

  return 0;
}
```

Output

```text
learned = true

map(train):
====================
     | amount  ratio
--------------------
0    | 0       0    
1    | 0.25    0.25 
2    | 0.5     0.5  
3    | 0.75    0.75 
4    | 1       1    
--------------------
[5 cs x 2 feat]
map(later) -- out-of-range rows go outside [0, 1]:
====================
     | amount  ratio
--------------------
0    | 0.125   0.125
1    | 0.625   0.625
2    | 1.25    1.25 
--------------------
[3 cs x 2 feat]
```

### Example 2

Reading the fitted parameters out of `summary().state`.

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

  // summary() on the macro-based scalers is a TransformerSummary: the pinned
  // feature_names plus a state vector of raw fitted tensors. For MinMaxScaler
  // state[1] is the learned minimum and state[2] the learned maximum
  // (state[0] is internal bookkeeping).
  lymph::prep::MinMaxScaler scaler;
  scaler.features({"amount"}).learn(train);
  const lymph::prep::TransformerSummary s = scaler.summary();
  std::cout << "feature_names: " << s.feature_names[0] << "\n";
  std::cout << "state tensors: " << s.state.size() << "\n";
  std::cout << "learned min = " << s.state[1].to_vec<double>()[0] << "\n";
  std::cout << "learned max = " << s.state[2].to_vec<double>()[0] << "\n";

  return 0;
}
```

Output

```text
feature_names: amount
state tensors: 3
learned min = 10
learned max = 50
```

### Example 3

`learn_map` in one call; with no `features()` it takes every numeric column.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

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

  // learn_map does learn() then map() and hands back the transformed frame.
  // With no features() call it takes every numeric column.
  lymph::prep::MinMaxScaler scaler;
  const DataFrame out = scaler.learn_map(train);
  std::cout << out << "\n";
  std::cout << "learned = " << std::boolalpha << scaler.learned() << "\n";

  return 0;
}
```

Output

```text
====================
     | amount  ratio
--------------------
0    | 0       0    
1    | 0.25    0.25 
2    | 0.5     0.5  
3    | 0.75    0.75 
4    | 1       1    
--------------------
[5 cs x 2 feat]
learned = true
```

## See Also

- [`prep::StandardScaler`](StandardScaler.md) — the shared method surface, documented in full.
- [`options::MinMaxScaler`](MinMaxScalerOptions.md) — the option fields.
- [`prep::stage::minmax_scaler`](../stage/minmax_scaler.md) — the factory twin; transform arithmetic and toggles.

## Changelog

- **0.1** — standalone `MinMaxScaler` with the shared `features` / `learn` / `map` / `learn_map` / `learned` / `summary` surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

MinMaxScalerOptions
```
<!-- apiref:toctree:end -->
