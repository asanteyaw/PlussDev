# `pluss::ai::ice::models` (models_nlp)

`models_nlp` is ice's text catalog: one entry,
[`TextJudge`](models_nlp/TextJudge.md), a text classifier built on
`nlp` `Doc`s/`msharp` ranges, plus its own `Options`/`TrainOptions`/
prediction/report types.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`TextJudgeOptions`](models_nlp/TextJudgeOptions.md) | hidden width, construction-time dropout, tokenization toggles for `TextJudge` |
| [`TextJudgeTrainOptions`](models_nlp/TextJudgeTrainOptions.md) | batch size/learning rate/optimizer/shared policies for `TextJudge::learn`, including a per-run dropout override |
| [`TextJudgePrediction`](models_nlp/TextJudgePrediction.md) | one predicted label plus its confidence, from `TextJudge::predict` |
| [`TextJudgeReport`](models_nlp/TextJudgeReport.md) | the loss/accuracy/count `TextJudge::assess` returns |
| [`TextJudge`](models_nlp/TextJudge.md) | the classifier itself: `learn`/`predict`/`assess`/`save`/`load`/`labels`/`ready`/`to` |

## How To Call It

Train a text judge on four labeled sentences, then predict labels for two new sentences. The program prints how many predictions came back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  models::TextJudgeOptions options{};
  options.width = 64;

  models::TextJudge model(options);
  const std::vector<std::string> texts = {"bake bread flour", "cook pasta sauce", "compile the kernel", "debug the parser"};
  const std::vector<std::string> labels = {"food", "food", "tech", "tech"};
  model.learn(texts, labels);

  const std::vector<std::string> new_texts = {"stir the sauce", "patch the compiler"};
  const auto preds = model.predict(new_texts);
  std::cout << "predictions = " << preds.size() << "\n";
  return 0;
}
```

Output

```text
predictions = 2
```

`learn`/`predict`/`assess` each take the text corpus in whichever of
four shapes is convenient (`Doc`, an `nlp`/`msharp` range view, a
plain `std::vector<std::string>`, or a `pluss::Note`) -- pick
whichever one the surrounding code already has on hand; all four
agree on the result for the same underlying texts.

## Related Pages

- [`models_ts`](models_ts.md) -- the other `models_*` family, sharing
  the same overall `*Options`/construct/`learn`/`predict`-`assess`
  calling convention, but not `TextJudge`'s own `to`/save-format
  specifics (see
  [`TextJudge`'s own Introduction](models_nlp/TextJudge/Introduction.md#to-is-weaker-here-than-in-models_ts)).
- [`options`](options.md) -- `ValidationPolicy`/
  `ArchitectureRegularization`/`ObjectiveRegularization`/
  `LoopRegularization`/`ExecutionPolicy`, the nested policy structs
  `TextJudgeTrainOptions` shares with
  [`TrainOptions`](options/TrainOptions.md).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

models_nlp/Introduction
models_nlp/TextJudgeOptions
models_nlp/TextJudgeTrainOptions
models_nlp/TextJudgePrediction
models_nlp/TextJudgeReport
models_nlp/TextJudge
```
<!-- apiref:toctree:end -->
