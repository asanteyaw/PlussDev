# `lymph::selection::spotcheck::Suite`

`Suite` is the **strategy × model spot-check driver** of the
[`selection::spotcheck`](../spotcheck.md) subfamily. It reads every model a
[`models::Builder`](../../models/builder/Builder.md) holds, crosses them with a
handful of training strategies, and ranks every combination by cross-validation
— the fastest way to answer "which modelling direction, under which training
settings, is worth taking further?" without writing a hyperparameter search.

## Scope And Alias

The type is `lymph::selection::spotcheck::Suite`, re-exported one scope up as
`lymph::selection::Suite`. Both names are the same type.

## Construction

```cpp
explicit Suite(const models::Builder& builder);
lymph::selection::Suite lymph::selection::suite(const models::Builder& builder);
```

Use the `suite(builder)` factory — it is the idiomatic spelling and reads left
to right with the chained `strategy` calls:

```cpp
namespace lymph = pluss::ai::lymph;

lymph::models::Builder registry = lymph::models::build({
    {"bayes", lymph::models::gaussian_nb_classifier()},
    {"tree",  lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
});

const lymph::selection::spotcheck::Result ranked =
    lymph::selection::suite(registry)
        .strategy("plain", lymph::TrainOptions{})
        .strategy("balanced", lymph::BalanceTarget{.kind = lymph::BalanceTargetKind::Equalize})
        .run(frame, data_options, cv);
```

The suite snapshots the builder's models at construction. A `Suite` with no
models, or with no strategy registered, raises at `run` time.

## Methods

| Method | Summary |
|--------|---------|
| [`strategy`](Suite/strategy.md) | Register one named training strategy — `(name, TrainOptions)`, `(name, BalanceTarget)`, or both. Chains. |
| [`run`](Suite/run.md) | Cross-validate every strategy × model candidate under one `CrossValOptions` and rank them → [`CompareResult`](../CompareReport.md). Overloads for `(frame, data_options)`, `LabeledBatch`, and `(Workflow, Payload)`. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Suite/Introduction
Suite/run
Suite/strategy
```
<!-- apiref:toctree:end -->

## Changelog

- **0.1** — `Suite` with the `suite(builder)` factory, `strategy` (three
  overloads), and `run` (`(frame, data_options)` / `LabeledBatch` /
  `(Workflow, Payload)`). `spotcheck::Result` is an alias of
  `selection::CompareResult`.
