# `pluss::ai::ProcedureOptions`

Procedure-level construction policy: worker count (parallel execution),
scheduling/caching intent, fail-fast behavior, and whether to validate
the graph immediately on construction.

Passed to [`procedure::make`](../build/make.md) (or the
[`Procedure`](Procedure.md) constructor directly) and read back via
[`Procedure::options`](Procedure/options.md); it never changes afterward,
since `Procedure` itself is immutable.

## Fields

| Field | Meaning | Default | When it matters |
| --- | --- | --- | --- |
| `workers` | Maximum number of independent steps that may execute concurrently | `1` | Every `run()` |
| `schedule` | Ordering policy within one ready wave | `ProcedureSchedule::Deterministic` | Reserved; both values currently use deterministic declaration order |
| `cache` | Artifact reuse policy | `ProcedureCache::Off` | Reserved; `ReadyArtifacts` currently performs no caching |
| `fail_fast` | Throw a step failure, or return it in `ProcedureResult` | `true` | Every `run()` failure |
| `validate_on_make` | Validate the graph during construction | `true` | `Procedure` construction |

### `workers`

`std::int64_t`, default `1`. The maximum number of steps
[`Procedure::run`](Procedure/run.md) may execute concurrently. At `1`
(the default), every run is fully serial and deterministic in wall-clock
order; above `1`, independent steps within the same topological wave run
across up to that many threads -- the *result* is identical either way,
only timing differs. Raise it only when a graph has real independent
branches worth parallelizing.

### `schedule`

`ProcedureSchedule`, default `Deterministic`. `Deterministic` runs steps
in declaration order within each topological wave. `Adaptive` is
reserved for a future scheduler that may reorder independent steps for
better throughput; it currently behaves identically to `Deterministic`.

### `cache`

`ProcedureCache`, default `Off`. `ReadyArtifacts` is reserved for a
future artifact cache keyed on upstream inputs; it currently has no
effect on [`run`](Procedure/run.md).

### `fail_fast`

`bool`, default `true`. At `true`, a step's exception propagates out of
[`run`](Procedure/run.md) immediately. At `false`, `run` instead returns
normally with [`ProcedureResult::ok`](ProcedureResult/ok.md) `false`,
[`failed_step`](ProcedureResult/failed_step.md) naming the step, and the
exception's message appended to
[`warnings`](ProcedureResult/warnings.md) -- set `false` for a production
caller that wants to inspect a partial result rather than catch an
exception.

### `validate_on_make`

`bool`, default `true`. At `true`, construction calls
[`check`](Procedure/check.md) immediately, so a `Procedure` value is
either valid or never exists. Set `false` only to construct a
deliberately-invalid graph for inspection via
[`check_message`](Procedure/check_message.md) -- every example on this
reference that needs to show a validation failure does exactly that.

## Guidance

The defaults (`workers = 1`, `schedule = Deterministic`, `cache = Off`,
`fail_fast = true`, `validate_on_make = true`) describe the simplest,
safest `Procedure`: fully serial, fully validated up front, and loud
about any failure. Raise `workers` for real parallelism opportunities;
flip `fail_fast` for a production caller that wants partial results
instead of exceptions; leave `validate_on_make` at `true` unless
specifically inspecting an invalid graph. `schedule`/`cache` are
forward-looking fields with no observable effect today -- setting them
is harmless but changes nothing yet.

`workers` must be positive. A zero or negative value fails graph validation;
it does not mean "choose automatically." Parallel execution is local to one
call to `run()`. It neither creates a persistent executor nor serializes
concurrent calls made by different caller threads.

## Examples

### Configure Parallel Capacity And Reserved Policies

```cpp
#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions options{};
  options.workers = 3;
  options.schedule = ai::ProcedureSchedule::Adaptive;
  options.cache = ai::ProcedureCache::ReadyArtifacts;

  const auto proc = ai::procedure::make({
      ai::procedure::step("flag", ai::StepOptions{
          .capabilities = {ai::StepCapability::Map},
          .outputs = {"ok"},
      }, [](const std::vector<ai::Artifact>&,
            const ai::RunOptions&) {
        return ai::Artifact("ok", ai::Artifact::Value(true));
      }),
  }, {}, options);

  std::cout << "workers=" << proc.options().workers << '\n';
  std::cout << "schedule="
            << static_cast<std::int64_t>(proc.options().schedule) << '\n';
  std::cout << "cache="
            << static_cast<std::int64_t>(proc.options().cache) << '\n';
}
```

Output:

```text
workers=3
schedule=1
cache=1
```

The values round-trip through `options()`. `workers` is active; the two enum
values reserve policy intent but do not yet alter execution.

### Return A Structured Failure

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions options{};
  options.fail_fast = false;

  const auto proc = ai::procedure::make({
      ai::procedure::step("needs_signal", "signal", "out",
          [](const pluss::Series& signal) { return signal; }),
  }, {}, options);

  const auto result = proc.run();
  std::cout << "ok=" << result.ok() << '\n';
  std::cout << "failed_step=" << result.failed_step() << '\n';
  std::cout << "warnings=" << result.warnings().size() << '\n';
}
```

Output:

```text
ok=0
failed_step=needs_signal
warnings=1
```

### Inspect An Invalid Graph Without Constructing It Twice

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions options{};
  options.validate_on_make = false;

  const auto cyclic = ai::procedure::make({
      ai::procedure::step("a"),
      ai::procedure::step("b"),
  }, {
      ai::procedure::link("a", "b"),
      ai::procedure::link("b", "a"),
  }, options);

  std::cout << "problem=" << cyclic.check_message().has_value() << '\n';
}
```

Output:

```text
problem=1
```

Disabling construction-time validation does not make an invalid graph
executable: `run()` calls `check()` before dispatch. This option exists for
inspection and diagnostics, not to bypass the graph contract.

## See Also

- [`procedure::make`](../build/make.md) -- where this is passed.
- [`Procedure::options`](Procedure/options.md) -- reading it back.
- [`RunOptions`](RunOptions.md) -- the per-call counterpart, as opposed
  to this per-construction policy.
