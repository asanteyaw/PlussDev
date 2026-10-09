<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ce4f264e41f9 -->
# `pluss::ai::ice::models::TextJudge::to`

[Back to `pluss::ai::ice::models::TextJudge`](../TextJudge.md)

## Signature

```cpp
pluss::ai::ice::models::TextJudge& pluss::ai::ice::models::TextJudge::to(
    pluss::ai::ice::Platform platform);
```

## What It Does

Updates this model's own [`TextJudgeOptions::cfg`](../TextJudgeOptions.md)
platform and returns `*this` so the call chains directly into the
next one (see Example 2). Unlike
[`AutoregressiveForecaster::to`](../../models_ts/AutoregressiveForecaster/to.md)/[`TemporalFusionTransformer::to`](../../models_ts/TemporalFusionTransformer/to.md),
this does **not** move any already-constructed parameter to a new
device in place -- `TextJudge`'s own classifier state is never
touched here, so a trained model's predictions are unaffected no
matter what platform is passed (see Example 1). The only place this
field is read again afterward is [`save`](save.md), which writes
whatever platform `to` last set into the artifact header -- a model
later [`load`](load.md)-ed from that artifact is reconstructed on
that platform, which is the practical way to move a `TextJudge`
across devices: `to(platform)` then `save`/`load`, not `to(platform)`
alone.

## Parameters

`Platform platform`
: The platform [`save`](save.md) should record for a future
  [`load`](load.md) to reconstruct on.

## Returns

`TextJudge&` -- `*this`, for chaining.

## Errors

None.

## Practical Examples

Three examples: `to()` leaving an already-trained model's predictions
completely unchanged, the returned reference chaining straight into
`predict`, and `ready()` surviving the call.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  std::vector<std::string> texts{};
  std::vector<std::string> labels{};
  for (int i = 0; i < 24; ++i) {
    texts.push_back("stocks rallied after the earnings report");
    labels.push_back("finance");
    texts.push_back("the market closed higher on strong profits");
    labels.push_back("finance");
    texts.push_back("the striker scored twice in the cup match");
    labels.push_back("sport");
    texts.push_back("fans celebrated the league victory downtown");
    labels.push_back("sport");
  }

  models::TextJudgeOptions model_options{};
  model_options.width = 24;
  model_options.dropout = 0.0;
  model_options.seed = 9;
  models::TextJudge model(model_options);

  models::TextJudgeTrainOptions train_options{};
  train_options.batch_size = 12;
  train_options.learning_rate = 5e-2;
  train_options.execution.max_epochs = 80;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 13;
  model.learn(texts, labels, train_options);

  const std::vector<std::string> probes{
      "profits lifted the stock market today",
      "the forward won the match with a late goal",
  };
  const auto before = model.predict(probes);
  model.to(pluss::pCPU);
  const auto after = model.predict(probes);

  std::cout << "to(pCPU) leaves predictions unchanged: "
            << (before[0].label == after[0].label && before[0].confidence == after[0].confidence &&
                before[1].label == after[1].label && before[1].confidence == after[1].confidence)
            << "\n";

  return 0;
}
```

Output

```text
to(pCPU) leaves predictions unchanged: 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  std::vector<std::string> texts{};
  std::vector<std::string> labels{};
  for (int i = 0; i < 24; ++i) {
    texts.push_back("stocks rallied after the earnings report");
    labels.push_back("finance");
    texts.push_back("the market closed higher on strong profits");
    labels.push_back("finance");
    texts.push_back("the striker scored twice in the cup match");
    labels.push_back("sport");
    texts.push_back("fans celebrated the league victory downtown");
    labels.push_back("sport");
  }

  models::TextJudgeOptions model_options{};
  model_options.width = 24;
  model_options.dropout = 0.0;
  model_options.seed = 9;
  models::TextJudge model(model_options);

  models::TextJudgeTrainOptions train_options{};
  train_options.batch_size = 12;
  train_options.learning_rate = 5e-2;
  train_options.execution.max_epochs = 80;
  train_options.validation.split_ratio = 0.0;
  train_options.seed = 13;
  model.learn(texts, labels, train_options);

  // to() returns *this by reference, so it chains directly into the next call.
  const auto preds = model.to(pluss::pCPU).predict(std::vector<std::string>{"alpha probe"});
  std::cout << "chained to().predict() size = " << preds.size() << "\n";

  return 0;
}
```

Output

```text
chained to().predict() size = 1
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;
  namespace models = ice::models;

  std::vector<std::string> texts{"alpha", "beta", "alpha", "beta"};
  std::vector<std::string> labels{"a", "b", "a", "b"};

  models::TextJudgeOptions model_options{};
  model_options.width = 8;
  models::TextJudge model(model_options);

  models::TextJudgeTrainOptions train_options{};
  train_options.execution.max_epochs = 3;
  train_options.validation.split_ratio = 0.0;
  model.learn(texts, labels, train_options);

  model.to(pluss::pCPU);
  std::cout << "ready() survives to(), still = " << model.ready() << "\n";

  return 0;
}
```

Output

```text
ready() survives to(), still = 1
```

## See Also

- [`ready`](ready.md) -- unaffected by `to`.
- [`save`](save.md) / [`load`](load.md) -- the actual mechanism for
  moving a `TextJudge` to a different platform.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save.html">
    <span class="label">Previous</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
