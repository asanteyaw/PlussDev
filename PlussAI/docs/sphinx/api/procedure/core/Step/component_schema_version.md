<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e5fee3e8d653 -->
# `pluss::ai::Step::component_schema_version`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
std::int64_t component_schema_version() const noexcept;
```

## What It Does

Returns `persist().component_schema_version` -- the version number for
the *wrapped component's own* saved format, as opposed to
[`persist_schema_version`](persist_schema_version.md), which versions the
bundle entry wrapping it. A [`registry`](../../registry.md) contract's
`CompatibilityGate` (when present) receives both numbers separately, so a
loader can accept a bundle entry layout change without requiring the
wrapped component's own format to have changed, and vice versa.

## Parameters

None.

## Returns

A `std::int64_t`, `0` by default for any `StepPersistSpec` that never set
it explicitly.

## Errors

None.

## Practical Examples

### Example 1

A sink step's default component schema version -- sinks have no
component format of their own to version, so this stays at its default.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "component_schema_version=" << s.component_schema_version() << "\n";
  return 0;
}
```

Output

```text
component_schema_version=0
```

### Example 2

An explicit value on a hand-attached
[`StepPersistSpec`](../StepPersistSpec.md).

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.key = "my_key";
  persist.component_schema_version = 3;
  const ai::Step s = ai::procedure::step("custom", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "component_schema_version=" << s.component_schema_version() << "\n";
  return 0;
}
```

Output

```text
component_schema_version=3
```

### Example 3

A different explicit value, independent of `persist_schema_version`.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.component_schema_version = 7;
  const ai::Step s = ai::procedure::step("versioned", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "component_schema_version=" << s.component_schema_version() << "\n";
  return 0;
}
```

Output

```text
component_schema_version=7
```

## See Also

- [`persist_schema_version`](persist_schema_version.md) -- the companion
  version for the bundle entry layout itself.
- [`registry`](../../registry.md) -- `CompatibilityGate`, where both
  numbers are checked together.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="component.html">
    <span class="label">Previous</span>
    <span class="title">component</span>
  </a>
  <a class="next-link" href="custom.html">
    <span class="label">Next</span>
    <span class="title">custom</span>
  </a>
</div>
<!-- apiref:nav:end -->
