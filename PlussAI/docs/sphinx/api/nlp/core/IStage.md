# `pluss::ai::nlp::IStage`

The contract every pipeline stage implements. A [`Workflow`](../workflow/Workflow.md)
holds an ordered list of `Stage` values (each a `std::shared_ptr<IStage>`),
and calls `apply` on every document in order. A stage that learns from
data also overrides `learn`, and a stage with persisted state overrides
`dump_state_lines` and `load_state_lines`. The built-in stages in
[`tasks`](../tasks.md) all implement this interface; a custom stage does
the same, and it then participates in pipelines, training, and saving
exactly like a built-in one.

## Signature

```cpp
pluss::ai::nlp::IStage::~IStage() = default;
```

## Construction

`IStage` is abstract: a caller never constructs one directly. A concrete
stage derives from it, overrides `name` and `apply` (the two pure virtual
members), and is handed to a `Workflow` as a shared pointer. Stages created
through the built-in factories (for example [`stage::tokenize`](../tasks/RuleTokenizerStage.md))
already return a `Stage`.

## State

A stage's configuration lives in its own fields, set by its constructor.
The base class holds no state. Learned state, when a stage has any, is the
stage's own business and reaches disk only through `dump_state_lines`.

## Methods

Each member has its own page:

| Method | Required | Covered on |
| --- | --- | --- |
| `name() const noexcept` | yes (pure virtual) | [`name`](IStage/name.md) |
| `apply(Doc&) const` | yes (pure virtual) | [`apply`](IStage/apply.md) |
| `learn(...)` | no (default does nothing) | [`learn`](IStage/learn.md) |
| `dump_state_lines() const` | no (default returns none) | [`dump_state_lines`](IStage/dump_state_lines.md) |
| `load_state_lines(...)` | no (default rejects state) | [`load_state_lines`](IStage/load_state_lines.md) |

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

IStage/Introduction
IStage/apply
IStage/dump_state_lines
IStage/learn
IStage/load_state_lines
IStage/name
```
<!-- apiref:toctree:end -->
