# `lymph::prep::StandardScaler`

`StandardScaler` is the standalone, fittable zero-mean/unit-variance scaler of
the `prep` family: construct it, [`learn`](#learn) column statistics on a frame,
then [`map`](#map) them onto any later frame. It is the object twin of the
[`prep::stage::standard_scaler`](../stage/standard_scaler.md) factory — same
arithmetic, same [`options::StandardScaler`](StandardScalerOptions.md), but a
value you hold and inspect rather than a step folded into a model.

## Scope And Alias

The type is `lymph::prep::StandardScaler`. It takes an
[`options::StandardScaler`](StandardScalerOptions.md) (family alias
`prep::StandardScalerOptions`) at construction, default-constructed if omitted.

```cpp
lymph::prep::StandardScaler a;                                   // defaults
lymph::prep::StandardScaler b{lymph::options::StandardScaler{.with_centering = false}};
```

## Construction

`explicit StandardScaler(options::StandardScaler options = {});`

Cheap — no data touched. The three option fields (`with_centering`,
`with_scaling`, `variance_floor`) and their effect on the arithmetic are covered
on the [`options::StandardScaler`](StandardScalerOptions.md) and
[`prep::stage::standard_scaler`](../stage/standard_scaler.md) pages.

## State

Before [`learn`](#learn) the object holds only its options.
[`learned()`](#learned) reads `false`. After `learn` it holds, per fitted column,
a mean and a scale — reachable through [`summary()`](#summary) as
`mean` / `scale` Series, and as the public members `mean_` / `scale_`. Fitting
again replaces that state.

## Methods

### `features`

```cpp
StandardScaler& features(std::initializer_list<std::string_view> names);
```

A second overload takes any range of string views in place of the braced list.

Pin the columns the scaler acts on, and fix their order. Returns `*this`, so it
chains into `learn`. **Optional** — call `learn` / `learn_map` without it and the
scaler takes every numeric column in the frame (see [`learn_map`](#learn_map)
Example). A pinned name that is absent from a later frame is skipped at
`map` time rather than raising.

### `learn`

```cpp
StandardScaler& learn(const pluss::table::DataFrame& frame,
                      const TrainOptions& options = {});
```

Fit the mean and scale of each targeted column from `frame`. Returns `*this` for
chaining, and flips [`learned()`](#learned) to `true`. Calling `learn` again
re-fits from scratch on the new frame — the object is a reusable template, not
single-shot.

#### Example

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
  const DataFrame later({{"amount", Thread(pluss::make_series({15., 35., 60.}, {3}, f64))},
                        {"ratio",  Thread(pluss::make_series({0.3, 0.7, 1.2}, {3}, f64))}});

  // The standalone object holds its fitted state. Pin the columns with
  // features(), fit with learn() (which returns *this so it chains), then apply
  // map() to any later frame -- the learned mean and scale are replayed.
  lymph::prep::StandardScaler scaler;
  std::cout << "learned before = " << std::boolalpha << scaler.learned() << "\n";

  scaler.features({"amount", "ratio"}).learn(train);
  std::cout << "learned after  = " << scaler.learned() << "\n\n";

  std::cout << "map(train):\n" << scaler.map(train) << "\n";
  std::cout << "map(later) -- same fitted parameters, new rows:\n" << scaler.map(later);

  return 0;
}
```

Output

```text
learned before = false
learned after  = true

map(train):
===================================
     | amount         ratio        
-----------------------------------
0    | -1.414213562   -1.414213562 
1    | -0.7071067812  -0.7071067812
2    |  0              0           
3    |  0.7071067812   0.7071067812
4    |  1.414213562    1.414213562 
-----------------------------------
[5 cs x 2 feat]
map(later) -- same fitted parameters, new rows:
===================================
     | amount         ratio        
-----------------------------------
0    | -1.060660172   -1.060660172 
1    |  0.3535533906   0.3535533906
2    |  2.121320344    2.121320344 
-----------------------------------
[3 cs x 2 feat]
```

### `map`

```cpp
DataFrame map(const pluss::table::DataFrame& frame,
              const InferOptions& options = {}) const;
```

Apply the fitted mean and scale to `frame`, returning a transformed copy — the
other columns pass through untouched. `frame` may have a different row count than
the training frame; a value outside the training range simply maps outside
`[-1, 1]` (row 2 above). `map` is `const` and does not consume the object, so the
same scaler serves many frames.

#### Example

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
  const DataFrame later({{"amount", Thread(pluss::make_series({15., 35., 60.}, {3}, f64))},
                        {"ratio",  Thread(pluss::make_series({0.3, 0.7, 1.2}, {3}, f64))}});

  // One fitted scaler, reused. Fit once on train; every later frame is
  // transformed against the same statistics -- the point of the object form
  // over a one-shot stage.
  lymph::prep::StandardScaler scaler;
  scaler.features({"amount", "ratio"}).learn(train);

  std::cout << "map(train):\n" << scaler.map(train) << "\n";
  std::cout << "map(later) -- same mean/scale, so row 2 (amount 60) lands past +2:\n"
            << scaler.map(later) << "\n";
  std::cout << "map(train) again -- identical, the object is not consumed:\n"
            << scaler.map(train);

  return 0;
}
```

Output

```text
map(train):
===================================
     | amount         ratio        
-----------------------------------
0    | -1.414213562   -1.414213562 
1    | -0.7071067812  -0.7071067812
2    |  0              0           
3    |  0.7071067812   0.7071067812
4    |  1.414213562    1.414213562 
-----------------------------------
[5 cs x 2 feat]
map(later) -- same mean/scale, so row 2 (amount 60) lands past +2:
===================================
     | amount         ratio        
-----------------------------------
0    | -1.060660172   -1.060660172 
1    |  0.3535533906   0.3535533906
2    |  2.121320344    2.121320344 
-----------------------------------
[3 cs x 2 feat]
map(train) again -- identical, the object is not consumed:
===================================
     | amount         ratio        
-----------------------------------
0    | -1.414213562   -1.414213562 
1    | -0.7071067812  -0.7071067812
2    |  0              0           
3    |  0.7071067812   0.7071067812
4    |  1.414213562    1.414213562 
-----------------------------------
[5 cs x 2 feat]
```

### `learn_map`

```cpp
DataFrame learn_map(const pluss::table::DataFrame& frame,
                    const TrainOptions& learn_options = {},
                    const InferOptions& infer_options = {});
```

`learn(frame)` then `map(frame)` in one call, returning the transformed frame.
The common shape when you only need the training frame transformed once. With no
[`features`](#features) call it fits — and transforms — every numeric column.

#### Example

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

  // learn_map fits and applies in one call, returning the transformed frame.
  // With no features() call the scaler acts on every numeric column.
  lymph::prep::StandardScaler scaler;
  const DataFrame out = scaler.learn_map(train);
  std::cout << out << "\n";
  std::cout << "columns fitted: " << scaler.summary().feature_names.size() << "\n";

  return 0;
}
```

Output

```text
===================================
     | amount         ratio        
-----------------------------------
0    | -1.414213562   -1.414213562 
1    | -0.7071067812  -0.7071067812
2    |  0              0           
3    |  0.7071067812   0.7071067812
4    |  1.414213562    1.414213562 
-----------------------------------
[5 cs x 2 feat]
columns fitted: 2
```

### `learned`

```cpp
bool learned() const noexcept;
```

`true` once [`learn`](#learn) or [`learn_map`](#learn_map) has run. Calling
[`map`](#map) before that is an error. Use it to guard a reused scaler, or to
assert a pipeline fitted as expected — see the transition in the
[`learn` example](#learn) (`false` → `true`).

### `summary`

```cpp
StandardScalerSummary summary() const;

struct StandardScalerSummary {
  std::vector<std::string> feature_names;
  Series mean;    // one value per fitted column
  Series scale;   // one value per fitted column
};
```

The fitted parameters. `StandardScaler` is the one scaler in this subfamily with
a **named** summary — `mean` and `scale` directly, rather than the opaque
`state` vector the others expose. `operator<<` on the summary prints the raw
`Series` representation; for a readable view, pull the values out with
`to_vec<double>()` alongside `feature_names`.

#### Example

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

  // summary() exposes the fitted parameters. StandardScaler's summary is a
  // dedicated struct with mean and scale as one-per-column Series.
  lymph::prep::StandardScaler scaler;
  scaler.features({"amount", "ratio"}).learn(train);
  const lymph::prep::StandardScalerSummary s = scaler.summary();

  const auto mean  = s.mean.to(pluss::pCPU, ScalarType::Float64).reshape({s.mean.numel()}).to_vec<double>();
  const auto scale = s.scale.to(pluss::pCPU, ScalarType::Float64).reshape({s.scale.numel()}).to_vec<double>();
  for (std::size_t i = 0; i < s.feature_names.size(); ++i) {
    std::cout << s.feature_names[i] << ": mean=" << mean[i] << " scale=" << scale[i] << "\n";
  }

  return 0;
}
```

Output

```text
amount: mean=30 scale=14.1421
ratio: mean=0.6 scale=0.282843
```

## Notes

- The scaler holds no reference to any frame — it is a snapshot of the fitted
  statistics. Mutating the training frame afterwards does not change a fitted
  scaler.
- `map` returns a new frame; it does not mutate its argument.
- To persist a fitted scaler as part of a model, attach the transform through
  [`Model::append`](../../models/Model/append.md) with
  [`prep::stage::standard_scaler`](../stage/standard_scaler.md) instead — the
  standalone object has no `save` / `load` of its own.

## Changelog

- **0.1** — standalone `StandardScaler` with `features` / `learn` / `map` /
  `learn_map` / `learned` / `summary`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

StandardScalerOptions
```
<!-- apiref:toctree:end -->
