# `pluss::ai::procedure` (core)

`core` owns the five types every `procedure` program is built from --
[`Procedure`](core/Procedure.md) (a validated DAG of steps and links),
[`Step`](core/Step.md) (one node), [`Link`](core/Link.md) (one edge),
[`Artifact`](core/Artifact.md) (the typed value flowing between steps), and
[`ProcedureResult`](core/ProcedureResult.md) (what `Procedure::run` returns)
-- plus the plain structs that configure and describe them.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Procedure`](core/Procedure.md) | the DAG itself: `check`/`run`, plus `steps`/`links`/`names`/`has`/`get` for inspecting it |
| [`Step`](core/Step.md) | one node's name, capabilities, declared ports, persistence spec, and `run` |
| [`Link`](core/Link.md) | one edge: `from`/`to`, plus optional named `from_output`/`to_input` ports |
| [`Artifact`](core/Artifact.md) | a named, type-erased value (`Series`, `DataFrame`, text, scalars, and every engine's own payload/report types) |
| [`ProcedureResult`](core/ProcedureResult.md) | every produced artifact, plus `trace`/`audit`/`warnings`/`failed_step` |
| [`ProcedureOptions`](core/ProcedureOptions.md) | `workers`/`schedule`/`cache`/`fail_fast`/`validate_on_make` -- `Procedure` construction policy |
| [`RunOptions`](core/RunOptions.md) | `mode`/`audit`/`allow_temporal_leakage` -- per-call `run()` policy |
| [`StepOptions`](core/StepOptions.md) | a step's declared contract: capabilities, ports, required columns, time/leakage flags |
| [`StepPersistSpec`](core/StepPersistSpec.md) | how one step saves itself into a bundle |
| [`SinkSpec`](core/SinkSpec.md) | a terminal step's one field: its target name |
| [`ArtifactTrace`](core/ArtifactTrace.md) | a lightweight shape/type snapshot of one artifact |
| [`StepTrace`](core/StepTrace.md) | one step's audit record: inputs, output, elapsed time, row-count change |

## How To Call It

Chain two steps with a link, run the graph once, and read the final artifact and the execution trace. The trace lists the steps in the order they ran.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/procedure.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) {
        return s.mul(10.0);
      }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) {
        return s.add(2.0);
      }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "score sum = " << out.get<pluss::Series>("score").sum().item<double>() << "\n";
  std::cout << "trace = [";
  const auto trace = out.trace();
  for (std::size_t i = 0; i < trace.size(); ++i) {
    std::cout << (i ? ", " : "") << "\"" << trace[i] << "\"";
  }
  std::cout << "]\n";
  return 0;
}
```

Output

```text
score sum = 66
trace = ["ran:scale", "ran:shift"]
```

Every `procedure` program follows this shape: build a list of
[`Step`](core/Step.md)s (by hand via [`Procedure`](core/Procedure.md)'s own
constructor, or -- far more commonly -- via the
[`procedure::step`](build/step.md) free function's many overloads, one per
engine adapter), a list of [`Link`](core/Link.md)s connecting them,
[`make`](build/make.md) a `Procedure`, then `run()` it with zero or more
named [`Artifact`](core/Artifact.md) inputs. `check()` (automatic on
`make()` unless [`ProcedureOptions::validate_on_make`](core/ProcedureOptions.md)
is set `false`) rejects a malformed graph -- cycles, duplicate names,
dangling links, uncapable steps -- before any step ever runs.

## Graph, Run, And Result Are Separate Layers

| Layer | Owns | Does not own |
| --- | --- | --- |
| `Procedure` | Immutable topology, construction policy, and step-captured component state | Inputs, outputs, or prior run history |
| `Step` | One executable node, declared contract, and optional persistence recipe | Global scheduling or another step's artifacts |
| `Artifact` | One named value in a particular run | Dependency order |
| `ProcedureResult` | Produced artifacts and diagnostics from one run | The reusable graph |

This separation prevents a common category error: a link does not carry a
value, and an artifact does not establish dependency order. Links decide when
a step is ready; names decide which values satisfy its ports.

## Capability Declarations

Capabilities describe a step's role in the graph: admission, mapping,
learning, inference, assessment, gating, joining, sinking, or watching. They
support validation, inspection, and deployment policy; adapter runners still
dispatch according to their own type and `RunOptions::mode`. A custom step may
declare several capabilities when its behavior spans roles.

## Failure Model

Construction failures are always exceptions because no valid graph exists to
return. Run-time failures follow `ProcedureOptions::fail_fast`: either the
exception escapes, or execution returns a partial `ProcedureResult` naming the
failed step and preserving earlier artifacts. `check_message()` is the
non-throwing structural diagnostic for callers intentionally inspecting an
invalid graph built with `validate_on_make=false`.

## Related Pages

- [`build`](build.md) -- `step`/`sink`/`link`/`input`/`make`, the free
  functions that construct every type on this page.
- [`callable`](callable.md), [`lymph`](lymph.md), [`ice`](ice.md),
  [`nlp`](nlp.md), [`module`](module.md) -- the five families of
  `procedure::step` overloads that wrap an engine component as a `Step`.
- [`registry`](registry.md) -- the custom-contract mechanism behind
  `StepPersistSpec::custom`.
- [`persist`](persist.md) -- the bundle format `Procedure::save`/
  `procedure::load` read and write.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

core/Introduction
core/Procedure
core/Step
core/Link
core/Artifact
core/ProcedureResult
core/ProcedureOptions
core/RunOptions
core/StepOptions
core/StepPersistSpec
core/SinkSpec
core/ArtifactTrace
core/StepTrace
```
<!-- apiref:toctree:end -->
