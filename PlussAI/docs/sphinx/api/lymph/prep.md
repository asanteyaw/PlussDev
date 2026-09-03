# Lymph `prep`

`prep` is the Lymph family for conditioning features before they reach a learner,
and for keeping that conditioning identical every time the model is used.

A raw feature frame is rarely ready for a learner as it stands. Columns arrive on
different scales, categories arrive as text or arbitrary integer codes, values go
missing, and some columns should be combined or dropped. Every one of those
adjustments has to be *learned once* on the training data — the column mean, the
category vocabulary, the imputation fill value — and then *replayed unchanged* on
every later frame, or training-time and inference-time features quietly disagree
and the model's accuracy erodes without an obvious cause.

`prep` is where that contract lives. It answers:

- which columns should be scaled, encoded, imputed, or passed through untouched
- what state each transformation must learn, and where that state is kept
- how a selection rule ("the numeric columns", "everything except the target")
  is written once and resolved repeatedly against real frames
- how the whole conditioning plan travels with a model so a caller passes raw
  features and never re-applies a transform by hand

## Where `prep` sits

| Family | Owns |
| --- | --- |
| `models` | the learner artifact and its fitted parameters |
| **`prep`** | **the feature-conditioning contract applied before the learner** |
| `selection` | the evidence policy that judges a learner |
| `workflow` | runtime and deployment-shaped orchestration around both |

`models` consumes `prep` — [`Model::append`](models/Model/append.md) and
[`Model::prepare`](models/Model/prepare.md) take a `prep::Stage` and a
`prep::Route` — but `prep` never depends on `models`. Every object in this family
can also be fitted and used entirely on its own, with no model in sight.

## The two entry points

`prep` exposes the same transformations through two surfaces, and which one you
reach for depends on whether the conditioning should belong to a model or stand
alone.

**Attachable planning — `prep::Route` + `prep::Stage`.** A
[`Route`](prep/route/Route.md) is a column selection; a
[`Stage`](prep/stage/Stage.md) is a transformation descriptor with its options
baked in. You pair them and hand the pair to a model:

```cpp
model.append(lymph::prep::stage::standard_scaler(),
             lymph::prep::route::names({"amount", "ratio"}));
```

Nothing is fitted at that point — the pair is an intention. The model fits it
during `learn_` and replays it during `infer` and `assess`. This is the everyday
path when preprocessing is part of what the model *is*.

**Standalone transformer objects — `prep::StandardScaler`,
`prep::OneHotEncoder`, `prep::FeatureTransformer`, and their siblings.** These are
classes you fit, inspect, and apply directly, holding their own learned state:

```cpp
lymph::prep::StandardScaler scaler;
scaler.learn(frame);              // learn column statistics
const DataFrame scaled = scaler.map(frame);   // replay them
```

Reach for these when a transformer is a first-class object in its own right — you
need to inspect its learned parameters, reuse it across several frames, or wire
it into something that is not a `lymph` model.

The two surfaces share the same option structs and the same underlying
transforms, so `prep::stage::standard_scaler(opts)` and
`prep::StandardScaler{opts}` do exactly the same arithmetic; only the return type
and the ownership differ.

## Subfamilies

- [`route`](prep/route.md) — declarative feature selection: `prep::route::names`,
  `prep::route::where`, `prep::route::remaining_of`, and the reusable
  `prep::Route` object they produce.
- [`stage`](prep/stage_family.md) — attachable transformation descriptors:
  `prep::Stage`, its `TransformContract`, and the `prep::stage::*` factory set.
- [`scaling`](prep/scaling.md) — standalone learned numeric transformers such as
  `prep::StandardScaler`.
- [`encoding`](prep/encoding.md) — standalone categorical transformers such as
  `prep::OneHotEncoder`.
- [`feature`](prep/feature.md) — many routed branches under one reusable
  `prep::FeatureTransformer`.

## End-to-end example

Two stages attached to a model — a scaler on the numeric columns, a one-hot
encoder on the categorical one — fitted once by `learn_` and replayed by both
`infer` and `assess`. The caller only ever passes the raw frame `x`.

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

  // Raw features on very different scales: a currency amount in the thousands
  // and a small ratio near 1. A categorical region code rides alongside.
  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"region", Thread(region)},
  });

  lymph::models::Model model = lymph::models::logistic_regression();

  // Two prep steps, each a stage descriptor paired with a route. The stage
  // says what to do; the route says which columns. Nothing is fitted yet.
  model.append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::names({"amount", "ratio"}));
  model.append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}));
  std::cout << "stage_count before learn = " << model.stage_count() << "\n";

  // learn_ resolves each route against the frame and fits each stage in order,
  // then fits the estimator on the conditioned features.
  model.learn_(x, y);
  std::cout << "stage_count after learn  = " << model.stage_count() << "\n";
  std::cout << "trained                  = " << std::boolalpha << model.trained() << "\n\n";

  // assess runs the same fitted prep contract before scoring, so callers pass
  // raw features and never re-apply the transforms by hand.
  lymph::options::Assess options{};
  options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};
  const lymph::MetricReport report = model.assess(x, y, options);

  const DataFrame review({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"region", Thread(region)},
      {"actual", Thread(y)},
      {"predicted", Thread(model.infer(x))},
  });
  std::cout << review << "\n";
  std::cout << "accuracy = " << report.accuracy.item<double>() << "\n";
  std::cout << "f1       = " << report.f1.item<double>() << "\n";

  return 0;
}
```

Output

```text
stage_count before learn = 0
stage_count after learn  = 2
trained                  = true

===============================================
     | amount  ratio  region  actual  predicted
-----------------------------------------------
0    | 1200    0.9    0       0       0        
1    | 1800    1.1    1       0       0        
2    | 3400    0.7    2       0       0        
3    | 4100    1.3    0       1       1        
4    | 5200    0.6    1       1       1        
5    | 6000    1.4    2       1       1        
-----------------------------------------------
[6 cs x 5 feat]

accuracy = 1
f1       = 1
```

The toy accuracy is not the point. The point is the ownership boundary: the prep
contract was attached before fitting, the fitted model reused it for inference,
and the same contract governed assessment — three call sites, one conditioning
plan, fitted exactly once.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

prep/route
prep/stage_family
prep/scaling
prep/encoding
prep/feature
```
<!-- apiref:toctree:end -->
