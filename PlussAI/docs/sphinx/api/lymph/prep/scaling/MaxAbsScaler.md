# `lymph::prep::MaxAbsScaler`

`MaxAbsScaler` is the standalone, fittable divide-by-magnitude scaler of the `prep` family: it learns each column's largest absolute value and divides by it, mapping the column into `[-1, 1]` without shifting zero.

## Scope And Alias

The type is `lymph::prep::MaxAbsScaler`. It takes an [`options::MaxAbsScaler`](MaxAbsScalerOptions.md) (family alias `prep::MaxAbsScalerOptions`) at construction, default-constructed if omitted. It shares its whole method surface with [`prep::StandardScaler`](StandardScaler.md) — `features` / `learn` / `map` / `learn_map` / `learned` / `summary`, with the same semantics — so that page is the reference for the mechanics. What follows is what is specific to `MaxAbsScaler`.

## Construction

`explicit MaxAbsScaler(MaxAbsScalerOptions options = {});` — cheap, no data touched. The option fields are on the [`options::MaxAbsScaler`](MaxAbsScalerOptions.md) page; the arithmetic and its toggles are on [`prep::stage::maxabs_scaler`](../stage/maxabs_scaler.md).

## Transform

For each fitted column `map` computes `x / max(|x|)`. Sign is preserved, a zero stays a zero, and a later value larger in magnitude than anything seen at fit time maps outside `[-1, 1]`.

## `summary()`

`MaxAbsScaler` returns a `TransformerSummary`, not a named struct:

```cpp
struct TransformerSummary {
  std::vector<std::string> feature_names;  // the pinned / fitted columns
  std::vector<Series> state;               // raw fitted tensors, layout below
};
```

`summary().state` holds two tensors. `state[1]` is the **learned maximum magnitude**, one value per fitted column; `state[0]` is internal bookkeeping. Example 2 reads it.

## Methods

The method surface is shared verbatim with [`prep::StandardScaler`](StandardScaler.md) — see that page for the full signatures, semantics, and per-method examples.

| Method | Purpose |
| --- | --- |
| [`features({...})`](StandardScaler.md#features) | pin the columns and their order (optional; without it, every numeric column) |
| [`learn(frame)`](StandardScaler.md#learn) | fit the maximum magnitude per column; returns `*this` |
| [`map(frame)`](StandardScaler.md#map) | divide each fitted column by its learned magnitude; `const` |
| [`learn_map(frame)`](StandardScaler.md#learn_map) | `learn` then `map` in one call |
| [`learned()`](StandardScaler.md#learned) | has `learn` run yet? |
| [`summary()`](StandardScaler.md#summary) | fitted parameters as a `TransformerSummary` (layout above) |

## Practical Examples

### Example 1

The shared lifecycle.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series amount = pluss::make_series({10., 20., 30., 40., 50.}, {5}, f64);
  const pluss::Series ratio  = pluss::make_series({0.2, 0.4, 0.6, 0.8, 1.0}, {5}, f64);
  const DataFrame train({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}});
  const DataFrame later({{"amount", Slate(pluss::make_series({15., 35., 60.}, {3}, f64))},
                        {"ratio",  Slate(pluss::make_series({0.3, 0.7, 1.2}, {3}, f64))}});

  lymph::prep::MaxAbsScaler scaler;
  scaler.features({"amount", "ratio"}).learn(train);
  std::cout << "learned = " << std::boolalpha << scaler.learned() << "\n\n";
  std::cout << "learn_map(train):\n" << lymph::prep::MaxAbsScaler{}.learn_map(train) << "\n";
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
0    | 0.2     0.2  
1    | 0.4     0.4  
2    | 0.6     0.6  
3    | 0.8     0.8  
4    | 1       1    
--------------------
[5 cs x 2 feat]
map(later):
====================
     | amount  ratio
--------------------
0    | 0.3     0.3  
1    | 0.7     0.7  
2    | 1.2     1.2  
--------------------
[3 cs x 2 feat]
```

### Example 2

Reading the learned magnitude out of `summary().state`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series amount = pluss::make_series({10., 20., 30., 40., 50.}, {5}, f64);
  const pluss::Series ratio  = pluss::make_series({0.2, 0.4, 0.6, 0.8, 1.0}, {5}, f64);
  const DataFrame train({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}});
  const DataFrame later({{"amount", Slate(pluss::make_series({15., 35., 60.}, {3}, f64))},
                        {"ratio",  Slate(pluss::make_series({0.3, 0.7, 1.2}, {3}, f64))}});

  // MaxAbsScaler learns a single value per column: the largest magnitude,
  // state[1].
  lymph::prep::MaxAbsScaler scaler;
  scaler.features({"amount"}).learn(train);
  const auto s = scaler.summary();
  std::cout << "state tensors: " << s.state.size() << "\n";
  std::cout << "learned max magnitude = " << s.state[1].to_vec<double>()[0] << "\n";

  return 0;
}
```

Output

```text
state tensors: 2
learned max magnitude = 50
```

## See Also

- [`prep::StandardScaler`](StandardScaler.md) — the shared method surface, documented in full.
- [`options::MaxAbsScaler`](MaxAbsScalerOptions.md) — the option fields.
- [`prep::stage::maxabs_scaler`](../stage/maxabs_scaler.md) — the factory twin; transform arithmetic and toggles.

## Changelog

- **0.1** — standalone `MaxAbsScaler` with the shared `features` / `learn` / `map` / `learn_map` / `learned` / `summary` surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

MaxAbsScalerOptions
```
<!-- apiref:toctree:end -->
