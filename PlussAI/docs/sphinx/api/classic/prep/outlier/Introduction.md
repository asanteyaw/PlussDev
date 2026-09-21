# `classic::outlier` Introduction

## What This Type Owns

[`OutlierIsolator`](OutlierIsolator.md) fits an isolation forest — an
ensemble of randomized trees that isolate each row by repeated random
splits — and scores every row by how *quickly* it gets isolated: a row
separated from the rest in very few splits is anomalous, one that takes
many splits to isolate is not. `score_samples` returns that continuous
score directly; `infer` thresholds it into a `-1` (anomaly) / `1` (normal)
label using a decision offset fitted alongside the forest.

## Why Not `TransformerBase`

Every other kernel across `prep` maps a feature matrix to another feature
matrix, or a target-shaped column, of the same row count. `OutlierIsolator`
maps rows to a *judgment about* those rows instead — there is no natural
"apply this transform" framing, so it exposes `learn` / `score_samples` /
`infer` / `learn_infer` as its own shape rather than conforming to `map`.

## Reading Order

Just the one page: [`OutlierIsolator`](OutlierIsolator.md).

## Related Pages

- [`prep`](../../prep.md) — the parent family.
- [`scaling`](../scaling.md) — `RobustScaler`, for de-emphasizing outliers
  in a fit rather than flagging the rows themselves.
