# `pluss::ai::procedure` (lymph)

`lymph` owns the three ways to wrap a real
[`lymph`](../plussai_lymph.md) component as a [`Step`](core/Step.md):
[`workflow::Policy`](../lymph/workflow/Policy.md),
[`models::Model`](../lymph/models/Model.md), and
[`workflow::Workflow`](../lymph/workflow/Workflow.md) -- all through one
overloaded [`step`](lymph/step.md) function.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`step`](lymph/step.md) | three overloads: a `Policy` transform step, a `Model` learn/infer/assess step, a `Workflow` learn/infer/assess step |

## How To Call It

Drop a column with a lymph workflow policy inside a procedure step. The program prints the run status and which columns survived the step.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  auto vec = [&](std::vector<double> values) {
    const auto n = static_cast<std::int64_t>(values.size());
    return pluss::make_series(std::move(values), {n}, cfg);
  };
  const DataFrame raw({
      {"id", Slate(vec({100.0, 101.0, 102.0}))},
      {"x", Slate(vec({1.0, 2.0, 3.0}))},
      {"y", Slate(vec({4.0, 5.0, 6.0}))},
  });

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("clean", "raw", "frame", lymph::workflow::Policy{.drop_columns = {"id"}}),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("raw", raw)});
  const DataFrame clean = out.get<DataFrame>("frame");
  std::cout << "ok = " << (out.ok() ? "true" : "false") << "\n";
  std::cout << "id kept = " << (clean.get("id").has_value() ? "true" : "false") << "\n";
  std::cout << "x kept = " << (clean.get("x").has_value() ? "true" : "false") << "\n";
  return 0;
}
```

Output

```text
ok = true
id kept = false
x kept = true
```

Every overload here is persistable by construction and routes on
[`RunOptions::mode`](core/RunOptions.md): `Policy` ignores it (it only
ever transforms), while `Model`/`Workflow` call `learn`/`infer`/`assess`
to match. A `Model`/`Workflow` step's `trained()` reflects the wrapped
component's own state at the moment the step was built, not at run time.

## Mode And Artifact Contract

| Adapter | Input | Output |
| --- | --- | --- |
| Policy | `DataFrame` or frame-bearing `workflow::Payload` | Same kind, transformed; payload roles preserved |
| Model/Workflow in `Learn` | `LabeledBatch` or labeled payload | Trained component |
| Model/Workflow in `Infer` | `FeatureBatch`, payload, or `DataFrame` | Prediction `Series` |
| Model/Workflow in `Assess` | `LabeledBatch` or labeled payload | `MetricReport` |

The wrapped value is moved into step-owned shared state. Learning through the
procedure updates that state for later runs through the same graph. The
trained component returned from a learning run is a value snapshot and can be
used to construct another graph explicitly.

Policy application to a payload uses `with_frame`: declared feature, label,
weight, group, and time roles remain attached. This is the important reason to
pass a payload instead of extracting its frame before admission.

## Persistence Boundary

Saved model/workflow steps include their train, inference, and assessment
options. Series-valued option fields are persisted as binary sidecars. Loading
validates both the procedure-side schema and the wrapped component's schema,
so a bundle cannot silently restore with a structurally different adapter.

## Related Pages

- [`build`](build.md) -- the generic `step` overloads this family
  layers on top of.
- [`ice`](ice.md) / [`nlp`](nlp.md) -- the sibling adapter families for
  `ice`/`nlp` components instead.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

lymph/Introduction
lymph/step
```
<!-- apiref:toctree:end -->
