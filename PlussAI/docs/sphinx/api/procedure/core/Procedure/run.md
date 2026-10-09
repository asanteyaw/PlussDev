<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ced48524a6a7 -->
# `pluss::ai::Procedure::run`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
ProcedureResult run(std::vector<Artifact> inputs = {}, RunOptions options = {}) const;
ProcedureResult run(std::initializer_list<Artifact> inputs, RunOptions options = {}) const;
```

## What It Does

Executes every step once, in topological order, and returns a
[`ProcedureResult`](../ProcedureResult.md). `inputs` seeds the artifact
pool with named values before any step runs (typically built with
[`procedure::input`](../../build/input.md)); each step's declared input
names are then resolved against that pool as upstream steps add their own
outputs to it. `check()` runs first, so a `Procedure` that somehow became
invalid after construction (not possible through the public API, since
`Procedure` is immutable, but relevant if a bundle was hand-edited before
[`load`](../../persist/load.md)) is still caught before any step executes.

If [`ProcedureOptions::workers`](../ProcedureOptions.md) is `1` (the
default), steps run serially on the calling thread in declaration order.
If `workers > 1`, independent steps (those with no dependency relationship
in the current wave of the topological order) run concurrently across up
to that many threads -- the result is identical either way; only the
wall-clock cost differs. `RunOptions::mode` controls which branch of each
adapter step's logic runs ([`ProcedureMode::Infer`](../../core.md) by
default); `RunOptions::audit` controls whether a
[`StepTrace`](../StepTrace.md) is recorded per step; `allow_temporal_leakage`
is the explicit override a step declaring
[`StepOptions::learns_from_future`](../StepOptions.md) needs in any mode
but `Infer`.

A step that throws stops the run: with
[`ProcedureOptions::fail_fast`](../ProcedureOptions.md) at its default
`true`, the exception propagates out of `run()`; with `fail_fast = false`,
`run()` instead returns normally with
[`ProcedureResult::ok`](../ProcedureResult/ok.md) `false`,
[`failed_step`](../ProcedureResult/failed_step.md) naming the step, and
the exception's message appended to
[`warnings`](../ProcedureResult/warnings.md) -- every artifact produced
before the failure is still present in the result.

## Parameters

`std::vector<Artifact> inputs` (or `std::initializer_list<Artifact>`)
: The named artifacts to seed the run with. Defaults to empty, for
  procedures whose first step needs no external input (a constant-valued
  [`callable`](../../callable.md) step, for instance).
`RunOptions options`
: Per-call policy -- `mode`/`audit`/`allow_temporal_leakage`. Defaults to
  `RunOptions{}` (`Infer` mode, audit on, leakage override off).

## Returns

A [`ProcedureResult`](../ProcedureResult.md) holding every artifact
produced, plus the trace/audit/warnings/failed-step bookkeeping.

## Errors

- Throws if the graph fails [`check`](check.md) (should not happen for a
  `Procedure` reached through the public API, since construction already
  validates it).
- With `fail_fast = true` (the default), propagates whatever a step's
  runner throws -- a missing/mistyped input, a failed
  [`StepOptions::require_columns`](../StepOptions.md)/time-feature check,
  or a blocked [`learns_from_future`](../StepOptions.md) step outside
  `allow_temporal_leakage`.
- With `fail_fast = false`, none of the above throw out of `run()`; they
  surface instead as `!result.ok()` plus `result.warnings()`.

## Practical Examples

### Example 1

A two-step `Series` pipeline run with one named input artifact.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) { return s.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  const std::vector<double> y = out.get<pluss::Series>("score").to(pluss::pCPU, pluss::ScalarType::Float64).to_vec<double>();
  std::cout << "score=[" << y[0] << "," << y[1] << "," << y[2] << "]\n";
  return 0;
}
```

Output

```text
score=[12,22,32]
```

### Example 2

`RunOptions::audit = false` skips per-step `StepTrace` bookkeeping while
`trace()` (the lightweight `"ran:<name>"` log) keeps recording regardless.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) { return s.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  ai::RunOptions options{};
  options.audit = false;
  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)}, options);
  std::cout << "trace=" << out.trace().size() << " audit=" << out.audit().size() << "\n";
  return 0;
}
```

Output

```text
trace=2 audit=0
```

### Example 3

Passing `RunOptions` explicitly with `mode` set -- here `Infer`, the
default, made explicit, which a plain `callable`/`module` step ignores
entirely since it has only one behavior.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) { return s.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  ai::RunOptions options{};
  options.mode = ai::ProcedureMode::Infer;
  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)}, options);
  std::cout << "ok=" << out.ok() << "\n";
  return 0;
}
```

Output

```text
ok=1
```

## See Also

- [`ProcedureResult`](../ProcedureResult.md) -- the return value, in
  full.
- [`RunOptions`](../RunOptions.md) -- every field of the second
  parameter.
- [`ProcedureOptions`](../ProcedureOptions.md) -- `workers`/`fail_fast`,
  the construction-time policy that shapes how `run()` behaves.
- [`procedure::input`](../../build/input.md) -- building one entry of
  `inputs`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="options.html">
    <span class="label">Previous</span>
    <span class="title">options</span>
  </a>
  <a class="next-link" href="save.html">
    <span class="label">Next</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
