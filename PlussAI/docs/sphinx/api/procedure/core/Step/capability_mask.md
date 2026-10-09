<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 472aa4b73299 -->
# `pluss::ai::Step::capability_mask`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
std::uint16_t capability_mask() const noexcept;
```

## What It Does

Folds `options().capabilities` into a single bitmask, OR-ing together
each declared [`StepCapability`](../Introduction.md)'s own bit (`1U <<
n`, in enumerator declaration order). [`has`](has.md) is built directly
on this. Most callers want `has()`; `capability_mask()` is for code that
needs to compare or store the whole set at once.

## Parameters

None.

## Returns

A `std::uint16_t` bitmask.

## Errors

None.

## Practical Examples

### Example 1

A single-capability `Map` step's mask is just that one bit (`1U << 1`).

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "mask=" << s.capability_mask() << "\n";
  return 0;
}
```

Output

```text
mask=2
```

### Example 2

A `Sink` step's mask is that capability's own bit (`1U << 7`).

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "mask=" << s.capability_mask() << "\n";
  return 0;
}
```

Output

```text
mask=128
```

### Example 3

Two capabilities together OR their bits: `Gate` (`1U << 5` = 32) and
`Map` (`1U << 1` = 2) combine to `34`.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Gate, ai::StepCapability::Map};
  const ai::Step s = ai::procedure::step("gated", options);

  std::cout << "mask=" << s.capability_mask() << "\n";
  return 0;
}
```

Output

```text
mask=34
```

## See Also

- [`has`](has.md) -- asking about one capability at a time.
- [`core`](../../core.md)'s own Introduction -- every `StepCapability`
  value and its conventional meaning.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="can_save_component.html">
    <span class="label">Previous</span>
    <span class="title">can_save_component</span>
  </a>
  <a class="next-link" href="component.html">
    <span class="label">Next</span>
    <span class="title">component</span>
  </a>
</div>
<!-- apiref:nav:end -->
