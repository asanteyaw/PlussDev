<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5530ae0d0f7f -->
# `pluss::ai::Procedure::options`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
const ProcedureOptions& options() const noexcept;
```

## What It Does

Returns the [`ProcedureOptions`](../ProcedureOptions.md) the procedure was
constructed with -- the same value passed to
[`make`](../../build/make.md) (or its default, if none was passed).
`Procedure` is immutable, so this never changes after construction.

## Parameters

None.

## Returns

A `const ProcedureOptions&`.

## Errors

None.

## Practical Examples

### Example 1

A procedure built with no explicit options gets `ProcedureOptions`'s
defaults -- `workers == 1`.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({ai::procedure::step("solo")});
  std::cout << "workers=" << proc.options().workers << "\n";
  return 0;
}
```

Output

```text
workers=1
```

### Example 2

Explicit `workers`/`schedule` round-trip through `options()` unchanged.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions opts{};
  opts.workers = 3;
  opts.schedule = ai::ProcedureSchedule::Adaptive;
  const ai::Procedure proc = ai::procedure::make({ai::procedure::step("solo")}, {}, opts);

  std::cout << "workers=" << proc.options().workers
             << " schedule=" << static_cast<int>(proc.options().schedule) << "\n";
  return 0;
}
```

Output

```text
workers=3 schedule=1
```

### Example 3

`cache`/`fail_fast` round-trip the same way.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions opts{};
  opts.cache = ai::ProcedureCache::ReadyArtifacts;
  opts.fail_fast = false;
  const ai::Procedure proc = ai::procedure::make({ai::procedure::step("solo")}, {}, opts);

  std::cout << "cache=" << static_cast<int>(proc.options().cache)
             << " fail_fast=" << proc.options().fail_fast << "\n";
  return 0;
}
```

Output

```text
cache=1 fail_fast=0
```

## See Also

- [`ProcedureOptions`](../ProcedureOptions.md) -- every field's meaning.
- [`run`](run.md) -- where `fail_fast` and `workers` take effect.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="names.html">
    <span class="label">Previous</span>
    <span class="title">names</span>
  </a>
  <a class="next-link" href="run.html">
    <span class="label">Next</span>
    <span class="title">run</span>
  </a>
</div>
<!-- apiref:nav:end -->
