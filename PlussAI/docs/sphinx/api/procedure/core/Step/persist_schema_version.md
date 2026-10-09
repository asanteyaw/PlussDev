<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f68e0d40db2f -->
# `pluss::ai::Step::persist_schema_version`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
std::int64_t persist_schema_version() const noexcept;
```

## What It Does

Returns `persist().schema_version` -- the version number a saved
manifest records for how this step's *bundle entry itself* is laid out
(distinct from [`component_schema_version`](component_schema_version.md),
which versions the wrapped component's own internal format).
[`procedure::load`](../../persist/load.md) compares this against the
registered loader's expectation and rejects an incompatible bundle rather
than silently misreading it.

## Parameters

None.

## Returns

A `std::int64_t`, `1` by default for any `StepPersistSpec` that never set
it explicitly.

## Errors

None.

## Practical Examples

### Example 1

A sink step's default schema version.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "schema_version=" << s.persist_schema_version() << "\n";
  return 0;
}
```

Output

```text
schema_version=1
```

### Example 2

An explicit, higher version on a hand-attached
[`StepPersistSpec`](../StepPersistSpec.md).

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.key = "my_key";
  persist.schema_version = 5;
  const ai::Step s = ai::procedure::step("custom", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "schema_version=" << s.persist_schema_version() << "\n";
  return 0;
}
```

Output

```text
schema_version=5
```

### Example 3

A different explicit value, to show it is an ordinary integer field, not
a fixed enum.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.schema_version = 2;
  const ai::Step s = ai::procedure::step("versioned", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "schema_version=" << s.persist_schema_version() << "\n";
  return 0;
}
```

Output

```text
schema_version=2
```

## See Also

- [`component_schema_version`](component_schema_version.md) -- the
  companion version for the wrapped component's own format.
- [`persist`](persist.md) -- the full spec this comes from.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="persist_key.html">
    <span class="label">Previous</span>
    <span class="title">persist_key</span>
  </a>
  <a class="next-link" href="persistable.html">
    <span class="label">Next</span>
    <span class="title">persistable</span>
  </a>
</div>
<!-- apiref:nav:end -->
