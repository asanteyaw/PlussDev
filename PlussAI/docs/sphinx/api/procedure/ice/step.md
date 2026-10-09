# `pluss::ai::procedure::step`

The two `ice` adapter overloads: a non-owning `ice::Model&` overload for
a model whose lifetime the caller keeps, and an owning template overload
(any `ice::Model` subclass) that takes a `restore_key`, making the step
persistable.

## Signature

```cpp
pluss::ai::Step pluss::ai::procedure::step(std::string name, std::string input_name, std::string output_name, ice::Model& model, ice::LossKind assess_loss = ice::LossKind::MSE, ice::TrainOptions train_options = {});

template <typename IceModel>
pluss::ai::Step pluss::ai::procedure::step(std::string name, std::string input_name, std::string output_name, IceModel model, std::string restore_key, ice::LossKind assess_loss = ice::LossKind::MSE, ice::TrainOptions train_options = {});
```

`IceModel` must derive from `ice::Model`.

## What It Does

Both overloads build a step with `Learn`/`Infer`/`Assess` capabilities
that routes on [`RunOptions::mode`](../core/RunOptions.md) exactly like
calling the model's own `learn`/`infer`/`assess` directly. The reference
overload never attaches a [`StepPersistSpec`](../core/StepPersistSpec.md)
-- the caller keeps the model alive outside the graph, so there is
nothing for `save`/`load` to restore. The owning overload moves `model`
into the step; with a non-empty `restore_key`, the step becomes
persistable -- `save` writes the model's parameters, and
[`load`](../persist/load.md) reconstructs a fresh instance via whatever
factory was bound for `restore_key` through
[`bind_ice`](../registry/bind_ice.md) before restoring the saved
weights. An empty `restore_key` leaves the owning overload's step
unpersistable too.

## Parameters

`std::string name`, `std::string input_name`, `std::string output_name`
: The step's name and its one input/output port.
`ice::Model& model`
: A model reference the caller continues to own.
`IceModel model`
: Any `ice::Model` subclass, moved into the step.
`std::string restore_key`
: A key previously bound via [`bind_ice`](../registry/bind_ice.md) (or
  empty, to opt out of persistence).
`ice::LossKind assess_loss`
: The loss [`assess`](../../ice/model/Model/assess.md) reports. Defaults
  to `MSE`.
`ice::TrainOptions train_options`
: Passed through to the model's own `learn`. Defaults to
  `TrainOptions{}`.

## Returns

A [`Step`](../core/Step.md) value.

## Errors

- Throws if `input_name` or `output_name` is empty.
- Throws (owning overload) if `name` is empty when `restore_key` is
  non-empty -- a persistable step needs a name.

## Practical Examples

### Example 1

The reference overload, learning a model the caller keeps.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace ice = pluss::ai::ice;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<float>{-2.0F, 0.0F, 3.0F}, {3, 1}, cfg);
  const pluss::Series target = pluss::make_series(std::vector<float>{0.0F, 0.0F, 3.0F}, {3, 1}, cfg);

  auto model = ice::Outline(pluss::nn::Fusion(1, 1));
  ice::TrainOptions train{};
  train.execution.max_epochs = 1;
  train.validation.split_ratio = 0.0;
  train.batch_size = 3;

  ice::Batch batch{};
  batch.set_inputs(x);
  batch.set_targets(target);
  ai::RunOptions learn{};
  learn.mode = ai::ProcedureMode::Learn;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("learn", "train", "done", static_cast<ice::Model&>(model), ice::LossKind::MSE, train),
  });
  const ai::ProcedureResult out = proc.run({ai::procedure::input("train", batch)}, learn);
  std::cout << "done=" << out.get<bool>("done") << "\n";
  return 0;
}
```

Output

```text
done=1
```

### Example 2

The owning overload with a bound `restore_key` makes the step
persistable.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <iostream>
#include <string>

int main() {
  namespace ai = pluss::ai;
  namespace ice = pluss::ai::ice;

  ai::procedure::bind_ice("doc_outline_v1", []() { return ice::Outline(pluss::nn::Fusion(1, 1)); });

  ice::TrainOptions train{};
  const ai::Step s = ai::procedure::step("learn", "train", "done",
      ice::Outline(pluss::nn::Fusion(1, 1)), std::string("doc_outline_v1"), ice::LossKind::MSE, train);

  std::cout << "persistable=" << s.persistable() << "\n";
  return 0;
}
```

Output

```text
persistable=1
```

### Example 3

The same owning overload with an empty `restore_key` opts out of
persistence.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <iostream>
#include <string>

int main() {
  namespace ai = pluss::ai;
  namespace ice = pluss::ai::ice;

  ice::TrainOptions train{};
  const ai::Step s = ai::procedure::step("learn", "train", "done",
      ice::Outline(pluss::nn::Fusion(1, 1)), std::string(""), ice::LossKind::MSE, train);

  std::cout << "persistable=" << s.persistable() << "\n";
  return 0;
}
```

Output

```text
persistable=0
```

## See Also

- [`bind_ice`](../registry/bind_ice.md) -- registering the factory a
  `restore_key` needs.
- [`module`](../module.md) -- the sibling escape hatch for raw `nn`
  objects that are not `ice::Model` subclasses.
