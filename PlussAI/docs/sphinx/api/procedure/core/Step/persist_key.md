<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 792efd45e5ae -->
# `pluss::ai::Step::persist_key`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
const std::string& persist_key() const noexcept;
```

## What It Does

Returns `persist().key` -- the identifier a saved bundle's manifest
records for this step's component, and the identifier
[`procedure::load`](../../persist/load.md) looks up (first among the
built-in loaders, then in the [`registry`](../../registry.md)'s bound
custom contracts) to reconstruct it.

## Parameters

None.

## Returns

A `const std::string&`, empty for a step with no persist spec attached.

## Errors

None.

## Practical Examples

### Example 1

A sink step's fixed built-in key.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "key=" << s.persist_key() << "\n";
  return 0;
}
```

Output

```text
key=procedure::Sink
```

### Example 2

A [`registry`](../../registry.md)-style custom key, attached directly
via [`StepPersistSpec`](../StepPersistSpec.md).

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.key = "my_key";
  const ai::Step s = ai::procedure::step("custom", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "key=" << s.persist_key() << "\n";
  return 0;
}
```

Output

```text
key=my_key
```

### Example 3

A plain callable step with no persist spec has an empty key.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "key=[" << s.persist_key() << "]\n";
  return 0;
}
```

Output

```text
key=[]
```

## See Also

- [`persist`](persist.md) -- the full spec this key comes from.
- [`procedure::load`](../../persist/load.md) -- what looks this key up.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="persist_fields.html">
    <span class="label">Previous</span>
    <span class="title">persist_fields</span>
  </a>
  <a class="next-link" href="persist_schema_version.html">
    <span class="label">Next</span>
    <span class="title">persist_schema_version</span>
  </a>
</div>
<!-- apiref:nav:end -->
