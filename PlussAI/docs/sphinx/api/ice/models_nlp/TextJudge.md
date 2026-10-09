# `pluss::ai::ice::models::TextJudge`

A text classifier built on `nlp` `Doc`s/`msharp` ranges -- the
heaviest input-type polymorphism in the catalog (`Doc`/`text_view`/
`string`/`Note` overloads throughout `learn`/`predict`/`assess`).

## Signature

```cpp
explicit pluss::ai::ice::models::TextJudge::TextJudge(
    pluss::ai::ice::models::TextJudgeOptions options = {});
```

## Construction

```cpp
namespace ice = pluss::ai::ice;
namespace models = ice::models;

models::TextJudgeOptions options{};
options.width = 64;
options.dropout = 0.1;

models::TextJudge model(options);
// or, with every field left at its default:
models::TextJudge default_model;
```

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `assess(texts, labels)` | `TextJudgeReport` | [`assess`](TextJudge/assess.md) |
| `labels()` | `const std::vector<std::string>&` | [`labels`](TextJudge/labels.md) |
| `learn(texts, labels, options = {})` | `void` | [`learn`](TextJudge/learn.md) |
| `load(filepath)` | `TextJudge` | [`load`](TextJudge/load.md) |
| `predict(texts)` | `std::vector<TextJudgePrediction>` | [`predict`](TextJudge/predict.md) |
| `ready()` | `bool` | [`ready`](TextJudge/ready.md) |
| `save(filepath)` | `void` | [`save`](TextJudge/save.md) |
| `to(platform)` | `TextJudge&` | [`to`](TextJudge/to.md) |

## Changelog

- **0.1** -- `TextJudge`: `learn`/`predict`/`assess` over `Doc`/
  `RangeView<std::string_view>`/`vector<std::string>`/`Note`,
  `save`/`load`, `labels`, `ready`, `to`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

TextJudge/Introduction
TextJudge/assess
TextJudge/labels
TextJudge/learn
TextJudge/load
TextJudge/predict
TextJudge/ready
TextJudge/save
TextJudge/to
TextJudgeOptions
TextJudgeTrainOptions
```
<!-- apiref:toctree:end -->
