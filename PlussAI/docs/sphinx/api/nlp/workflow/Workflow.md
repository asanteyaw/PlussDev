# `pluss::ai::nlp::Workflow`

The central pipeline object: an ordered stage list plus `learn`, `infer`,
`assess`, `save`, and `load`. It runs against raw text, a `pluss::Note` of
text, or already-tokenized documents.

## Signature

```cpp
pluss::ai::nlp::Workflow::Workflow() = default;
```

## Construction

`Workflow` is an aggregate. Set the language and the stages by name:

```cpp
nlp::Workflow wf{
    .lang = nlp::Language::English,
    .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()},
};
```

Stages come from the factories in [`tasks`](../tasks.md) (`stage::tokenize`,
`stage::sentencize`, and so on). A custom stage is added the same way, as a
shared pointer to an [`IStage`](../core/IStage.md) subclass.

## State

`lang`
: The `Language` used by language-specific stages (abbreviation lists,
  lemma tables). Default `English`.

`stages`
: The ordered list of stages, each a shared pointer to an
  [`IStage`](../core/IStage.md). Stages run in this order on every call.
  [`append_stage`](Workflow/append_stage.md) adds to the end.

A workflow holds no documents. Results come back from each call.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `append_stage(Stage)` | `Workflow&` | [`append_stage`](Workflow/append_stage.md) |
| `learn(...)` (8 overloads) | `Workflow&` | [`learn`](Workflow/learn.md) |
| `infer(...)` (4 overloads) | `Doc` / `std::vector<Doc>` | [`infer`](Workflow/infer.md) |
| `assess(...)` (4 overloads) | `WorkflowReport` | [`assess`](Workflow/assess.md) |
| `save(path) const` | `void` | [`save`](Workflow/save.md) |
| `load(path)` (static) | `Workflow` | [`load`](Workflow/load.md) |

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Workflow/Introduction
Workflow/append_stage
Workflow/assess
Workflow/infer
Workflow/learn
Workflow/load
Workflow/save
```
<!-- apiref:toctree:end -->
