# `pluss::ai::procedure` (ice)

`ice` owns the two ways to wrap an [`ice::Model`](../ice/model/Model.md)
subclass as a [`Step`](core/Step.md): a non-owning reference overload,
and an owning template overload that takes a `restore_key` for
persistence.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`step`](ice/step.md) | two overloads: `ice::Model&` by reference (unpersisted), or any `ice::Model` subclass owned-with-`restore_key` (persistable) |

## How To Call It

Wrap an ice model in a learn step, then run the procedure in learn mode. The program prints the run status and how many epochs the model trained. The four-row data set sets `validation.split_ratio` to 0.0, because the default split would leave no validation rows.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/procedure.h>
#include <pluss/ai/ice.h>
#include <pluss/ai/lymph.h>
#include <pluss/nn.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace ice = pluss::ai::ice;
  namespace lymph = pluss::ai::lymph;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4, 1}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{2.0, 4.0, 6.0, 8.0}, {4, 1}, cfg);
  const lymph::LabeledBatch train{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };

  auto model = ice::Outline(pluss::nn::Fusion(1, 1));
  ice::TrainOptions options{};
  options.loss = ice::LossKind::MSE;
  options.learning_rate = 0.05;

  options.batch_size = 2;
  options.validation.split_ratio = 0.0;
  options.execution.max_epochs = 3;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("learn", "train", "done", static_cast<ice::Model&>(model), ice::LossKind::MSE, options),
  });

  ai::RunOptions learn{};
  learn.mode = ai::ProcedureMode::Learn;
  const ai::ProcedureResult out = proc.run({ai::procedure::input("train", train)}, learn);

  std::cout << "ok = " << (out.ok() ? "true" : "false") << "\n";
  std::cout << "epochs trained = " << model.training_report().epochs.size() << "\n";
  return 0;
}
```

Output

```text
ok = true
epochs trained = 3
```

Both overloads route on [`RunOptions::mode`](core/RunOptions.md) exactly
like calling the model's own `learn`/`infer`/`assess` directly would.
Reach for the owning overload with [`bind_ice`](registry/bind_ice.md)
only when the graph itself should own and persist the model; keep the
reference overload for a model trained and managed outside the graph.

## Mode And Artifact Contract

| Mode | Accepted input | Published output |
| --- | --- | --- |
| `Infer` | `Series`, `lymph::FeatureBatch`, or input-bearing `ice::Batch` | Prediction `Series` |
| `Learn` | `lymph::LabeledBatch`, or `ice::Batch` with inputs and targets | `bool` success marker |
| `Assess` | `lymph::LabeledBatch`, or `ice::Batch` with inputs and targets | `ice::AssessReport` |

The adapter does not reinterpret an incomplete batch. Learning and assessment
require both inputs and targets; inference requires inputs. Type and presence
checks occur before calling the model.

## Ownership Choice

The reference overload stores a non-owning `shared_ptr` facade with a no-op
deleter. It never extends the model's lifetime: moving or destroying the model
while the step remains usable leaves an invalid graph. Its advantage is that
learning through the procedure updates the exact model object held by the
caller.

The value overload moves the concrete model into owned shared state. The
original object is no longer needed, and procedure copies continue to refer to
the same captured model state. Supply an empty restore key for ownership
without persistence; supply a bound key when `save()`/`load()` must recreate
the concrete subclass before restoring its weights.

## Performance Boundary

Procedure adds artifact dispatch and audit metadata around the model call; it
does not move model data to a different platform or insert host copies. Input
Series retain their platform, and the wrapped model remains responsible for
its own batching, device placement, and training execution policy.

## Related Pages

- [`ice::Model`](../ice/model/Model.md) -- the base class every wrapped
  model derives from.
- [`registry`](registry.md) -- `bind_ice`, the factory registry the
  owning overload's `restore_key` depends on.
- [`module`](module.md) -- the raw-module escape hatch for non-`ice::Model`
  `nn` objects.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

ice/Introduction
ice/step
```
<!-- apiref:toctree:end -->
