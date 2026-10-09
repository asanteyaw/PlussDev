<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 973a4cf34c82 -->
# `pluss::ai::Step::can_change_row_count`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
bool can_change_row_count() const noexcept;
```

## What It Does

Returns `options().can_change_row_count` -- whether this step is allowed
to produce an output whose row count differs from its input's.
[`Procedure::run`](../Procedure/run.md) compares each step's recorded
input/output row counts (via [`ArtifactTrace::rows`](../ArtifactTrace.md))
and throws if a row-count change is detected on a step that did not
declare this flag -- a safety net against a silent row-dropping bug
reaching a sink unnoticed.

## Parameters

None.

## Returns

`true` or `false`.

## Errors

None.

## Practical Examples

### Example 1

A step that only transforms values, without dropping or adding rows,
leaves this at its default `false`.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "can_change_row_count=" << s.can_change_row_count() << "\n";
  return 0;
}
```

Output

```text
can_change_row_count=0
```

### Example 2

A step that legitimately filters/resamples rows declares the flag
explicitly.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepOptions options{};
  options.can_change_row_count = true;
  const ai::Step s = ai::procedure::step("shrink", options);

  std::cout << "can_change_row_count=" << s.can_change_row_count() << "\n";
  return 0;
}
```

Output

```text
can_change_row_count=1
```

### Example 3

A sink step does not reshape anything, so it leaves the flag at its
default too.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "can_change_row_count=" << s.can_change_row_count() << "\n";
  return 0;
}
```

Output

```text
can_change_row_count=0
```

## See Also

- [`Procedure::run`](../Procedure/run.md) -- where this flag is enforced.
- [`StepTrace::row_count_changed`](../StepTrace.md) -- the per-run audit
  flag this guard is built on.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="can_save_component.html">
    <span class="label">Next</span>
    <span class="title">can_save_component</span>
  </a>
</div>
<!-- apiref:nav:end -->
