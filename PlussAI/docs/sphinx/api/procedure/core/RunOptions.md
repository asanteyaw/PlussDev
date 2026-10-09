# `pluss::ai::RunOptions`

Per-run policy passed to [`Procedure::run`](Procedure/run.md): which
`ProcedureMode` to execute in, whether to collect a `StepTrace` audit,
and whether to allow steps that declare future-data access.

Unlike [`ProcedureOptions`](ProcedureOptions.md) (fixed once, at
construction), `RunOptions` is supplied fresh to every `run()` call --
the same `Procedure` can be run in `Infer` mode for serving and `Learn`
mode for (re)training without being rebuilt.

## Fields

| Field | Meaning | Default | Read by |
| --- | --- | --- | --- |
| `mode` | Select inference, learning, or assessment behavior | `ProcedureMode::Infer` | Model-aware `lymph`, `ice`, and `nlp` adapters |
| `audit` | Collect structured `StepTrace` records | `true` | `Procedure::run()` |
| `allow_temporal_leakage` | Explicitly permit a future-reading step outside inference | `false` | The central temporal safety guard |

### `mode`

`ProcedureMode`, default `Infer`. Selects which branch of each adapter
step's own logic runs: `Infer` calls the wrapped component's inference
path, `Learn` calls its learning path, `Assess` calls its evaluation
path. A plain [`callable`](../callable.md)/[`module`](../module.md) step
ignores `mode` entirely, since it has only one behavior.

### `audit`

`bool`, default `true`. At `true`, [`run`](Procedure/run.md) records a
full [`StepTrace`](StepTrace.md) per step into
[`ProcedureResult::audit`](ProcedureResult/audit.md). At `false`, that
bookkeeping is skipped -- [`trace`](ProcedureResult/trace.md)'s cheaper
`"ran:<name>"` log still records regardless.

### `allow_temporal_leakage`

`bool`, default `false`. A step declaring
[`StepOptions::learns_from_future`](StepOptions.md) is blocked in any
mode but `Infer` unless this is explicitly set `true` -- the deliberate
override an audited backtest needs to intentionally let a step see
future data, with the override itself visible at the call site rather
than buried in the step's own definition.

## Guidance

Leave `mode` at `Infer` for ordinary serving; set it to `Learn`/`Assess`
only when actually training or evaluating the graph's own models.
`audit` trades a small amount of per-step bookkeeping cost for a full
audit trail -- leave it `true` unless the trail is genuinely unneeded for
a hot path. `allow_temporal_leakage` should stay `false` everywhere
except an explicitly audited backtest; its default being `false` is the
safety rail [`StepOptions::learns_from_future`](StepOptions.md) is built
around.

The options object is copied into a run. Changing the caller's object after
`run()` begins cannot change an in-flight execution, and no setting is retained
as a new default on the `Procedure`.

## Examples

### Disable Structured Audit Without Disabling The Trace

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}
      .with_dtype(pluss::ScalarType::Float64)
      .with_platform(pluss::pCPU);
  const auto x = pluss::make_series(
      std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);
  const auto proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled",
          [](const pluss::Series& value) { return value.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score",
          [](const pluss::Series& value) { return value.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  ai::RunOptions options{};
  options.audit = false;
  const auto result = proc.run({ai::procedure::input("x", x)}, options);
  std::cout << "trace=" << result.trace().size() << '\n';
  std::cout << "audit=" << result.audit().size() << '\n';
}
```

For a two-step graph, the output is:

```text
trace=2
audit=0
```

The trace is a cheap execution log. Audit records additionally snapshot input
and output metadata and elapsed time.

### Select Learning Explicitly

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;

  const auto cfg = pluss::Config{}
      .with_dtype(pluss::ScalarType::Float64)
      .with_platform(pluss::pCPU);
  const lymph::LabeledBatch batch{
      .features = lymph::FeatureBatch{
          .x = pluss::make_series(
              std::vector<double>{0.0, 1.0, 1.0, 2.0,
                                  2.0, 3.0, 3.0, 4.0},
              {4, 2}, cfg),
          .layout = lymph::ObservationLayout::Rows,
      },
      .y = pluss::make_series(
          std::vector<double>{1.0, 3.0, 5.0, 7.0}, {4}, cfg),
  };
  const auto trainer = ai::procedure::make({
      ai::procedure::step(
          "fit", "train", "model",
          lymph::models::linear_regression()),
  });

  ai::RunOptions options{};
  options.mode = ai::ProcedureMode::Learn;
  const auto result = trainer.run({
      ai::procedure::input("train", batch),
  }, options);
  const auto& model = result.get<lymph::models::Model>("model");
  std::cout << "trained=" << model.trained() << '\n';
}
```

Output:

```text
trained=1
```

The mode changes the adapter's result type: this learning call publishes the
trained model, whereas inference publishes predictions and assessment
publishes a report.

### Make A Future-Reading Training Step Deliberate

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}
      .with_dtype(pluss::ScalarType::Float64)
      .with_platform(pluss::pCPU);
  const auto x = pluss::make_series(
      std::vector<double>{1.0, 2.0}, {2}, cfg);

  ai::StepOptions step_options{};
  step_options.inputs = {"x"};
  step_options.outputs = {"out"};
  step_options.learns_from_future = true;

  ai::ProcedureOptions procedure_options{};
  procedure_options.fail_fast = false;
  const auto proc = ai::procedure::make({
      ai::procedure::step(
          "lookahead", step_options,
          [](const std::vector<ai::Artifact>& inputs,
             const ai::RunOptions&) {
            return ai::Artifact(
                "out",
                ai::Artifact::Value(
                    inputs.front().get<pluss::Series>()));
          }),
  }, {}, procedure_options);

  ai::RunOptions blocked{};
  blocked.mode = ai::ProcedureMode::Learn;
  ai::RunOptions allowed = blocked;
  allowed.allow_temporal_leakage = true;

  const auto first =
      proc.run({ai::procedure::input("x", x)}, blocked);
  const auto second =
      proc.run({ai::procedure::input("x", x)}, allowed);
  std::cout << "blocked=" << first.ok() << '\n';
  std::cout << "allowed=" << second.ok() << '\n';
}
```

With a non-fail-fast procedure whose step declares
`learns_from_future=true`, the output is:

```text
blocked=0
allowed=1
```

The override belongs at the call site so an exceptional backtest cannot
silently become the graph's permanent policy.

## See Also

- [`Procedure::run`](Procedure/run.md) -- the only consumer.
- [`ProcedureOptions`](ProcedureOptions.md) -- the per-construction
  counterpart.
- [`StepOptions`](StepOptions.md) -- `learns_from_future`'s own page.
