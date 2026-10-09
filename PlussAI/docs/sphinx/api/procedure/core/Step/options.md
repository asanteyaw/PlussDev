<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8c0417339c0a -->
# `pluss::ai::Step::options`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
const StepOptions& options() const noexcept;
StepOptions& options() noexcept;
```

## What It Does

Returns the full [`StepOptions`](../StepOptions.md) the step was built
with. Every other contract accessor (`inputs`/`outputs`/`persistable`/
`trained`/`can_change_row_count`/`component`/`capability_mask`/`has`) is a
thin, read-only forward over one field of this struct; `options()` is the
escape hatch when a caller needs a field none of those expose directly,
such as `require_columns` or `time_feature`. The mutable overload lets a
caller already holding a non-`const` `Step` adjust its contract in place
-- rare, since steps are normally built fully-formed by
[`procedure::step`](../../build/step.md).

## Parameters

None.

## Returns

A `StepOptions&` (or `const StepOptions&` on a `const Step`).

## Errors

None.

## Practical Examples

### Example 1

Reading a field `options()` exposes that has no dedicated accessor --
`inputs.size()`.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "inputs=" << s.options().inputs.size() << "\n";
  return 0;
}
```

Output

```text
inputs=1
```

### Example 2

`capabilities.size()` -- how many capabilities this step declared.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "capabilities=" << s.options().capabilities.size() << "\n";
  return 0;
}
```

Output

```text
capabilities=1
```

### Example 3

`component`, the same string [`component()`](component.md) forwards.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "component=" << s.options().component << "\n";
  return 0;
}
```

Output

```text
component=callable
```

## See Also

- [`StepOptions`](../StepOptions.md) -- every field's meaning.
- [`inputs`](inputs.md) / [`outputs`](outputs.md) -- the two fields most
  callers actually need.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="name.html">
    <span class="label">Previous</span>
    <span class="title">name</span>
  </a>
  <a class="next-link" href="outputs.html">
    <span class="label">Next</span>
    <span class="title">outputs</span>
  </a>
</div>
<!-- apiref:nav:end -->
