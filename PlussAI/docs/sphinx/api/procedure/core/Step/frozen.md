<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2dcf94ea9c7a -->
# `pluss::ai::Step::frozen`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
bool frozen() const noexcept;
```

## What It Does

Returns `persist().frozen` -- whether this step's persisted component is
expected to never change between saves. [`registry`](../../registry.md)'s
`CustomContract::frozen` and `CustomPersist::frozen` both feed into this;
a frozen component is a signal (to a caller inspecting a bundle, or to a
future compatibility check) that re-saving should produce byte-identical
output, not that `save`/`load` themselves behave any differently today.

## Parameters

None.

## Returns

`true` or `false`.

## Errors

None.

## Practical Examples

### Example 1

A sink step is not frozen -- its target is ordinary, mutable
configuration.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "frozen=" << s.frozen() << "\n";
  return 0;
}
```

Output

```text
frozen=0
```

### Example 2

A hand-attached [`StepPersistSpec`](../StepPersistSpec.md) with
`frozen = true` set explicitly.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.key = "my_key";
  persist.frozen = true;
  const ai::Step s = ai::procedure::step("custom", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "frozen=" << s.frozen() << "\n";
  return 0;
}
```

Output

```text
frozen=1
```

### Example 3

A plain callable step with no persist spec reads `false` by default,
same as any other unset `StepPersistSpec` field.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "frozen=" << s.frozen() << "\n";
  return 0;
}
```

Output

```text
frozen=0
```

## See Also

- [`StepPersistSpec`](../StepPersistSpec.md) -- the full struct.
- [`registry`](../../registry.md) -- `CustomContract`/`CustomPersist`'s
  own `frozen` fields.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="custom.html">
    <span class="label">Previous</span>
    <span class="title">custom</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
<!-- apiref:nav:end -->
