<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 97293f78b3d6 -->
# `pluss::ai::Procedure::step_count`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
std::int64_t step_count() const noexcept;
```

## What It Does

Returns `steps().size()` as a signed `std::int64_t`, matching the
integer type the rest of `procedure` uses for counts (`StepTrace::elapsed_us`,
`ArtifactTrace::rows`, and so on) rather than `size_t`.

## Parameters

None.

## Returns

A non-negative `std::int64_t`.

## Errors

None.

## Practical Examples

### Example 1

A two-step, one-link procedure reports `step_count() == 2`.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) { return s.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  std::cout << "step_count=" << proc.step_count() << "\n";
  return 0;
}
```

Output

```text
step_count=2
```

### Example 2

A three-step chain reports `3`, regardless of how many links connect them.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("a"), ai::procedure::step("b"), ai::procedure::step("c"),
  }, {
      ai::procedure::link("a", "b"), ai::procedure::link("b", "c"),
  });

  std::cout << "step_count=" << proc.step_count() << "\n";
  return 0;
}
```

Output

```text
step_count=3
```

### Example 3

A single-step procedure reports `1`.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({ai::procedure::step("solo")});
  std::cout << "step_count=" << proc.step_count() << "\n";
  return 0;
}
```

Output

```text
step_count=1
```

## See Also

- [`steps`](steps.md) -- the full vector this counts.
- [`link_count`](link_count.md) -- the edge-side equivalent.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save.html">
    <span class="label">Previous</span>
    <span class="title">save</span>
  </a>
  <a class="next-link" href="steps.html">
    <span class="label">Next</span>
    <span class="title">steps</span>
  </a>
</div>
<!-- apiref:nav:end -->
