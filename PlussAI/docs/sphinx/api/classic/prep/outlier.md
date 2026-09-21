# `pluss::ai::classic::prep::outlier`

`outlier` owns scoring and flagging anomalous rows — a different shape of
problem from every other `prep` sub-family, which all *reshape column
values*. `outlier` instead asks a per-row question ("how unusual is this
row, relative to the training data as a whole?") and answers it with a
score or a label, not a transformed feature matrix.

Today this is one type: [`OutlierIsolator`](outlier/OutlierIsolator.md), an
isolation-forest anomaly detector. It is **not** a `TransformerBase` — its
public shape is `learn` / `score_samples` / `infer` / `learn_infer` rather
than `learn` / `map` / `learn_map`, reflecting that its output is a score or
a label, not a re-expression of the input features.

## How To Call It

```cpp
namespace cls = pluss::ai::classic;

cls::prep::OutlierIsolator isolator;  // contamination = auto by default
isolator.learn(train_batch);

const pluss::Series scores = isolator.score_samples(probe_batch).value();  // continuous, lower = more anomalous
const pluss::Series labels = isolator.infer(probe_batch).value();          // -1 (anomaly) or 1 (normal)
```

## Related Pages

- [`prep`](../prep.md) — the other four transform sub-families.
- [`scaling`](scaling.md) — `RobustScaler`, a *de-emphasizing* alternative
  to outliers rather than flagging them outright.
- [`families`](../families.md) — `DetectFamilyOptions`, the `classic`
  estimator-shaped path to anomaly detection (via `make_transformer`),
  distinct from this standalone type.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

outlier/Introduction
outlier/OutlierIsolator
```
<!-- apiref:toctree:end -->
