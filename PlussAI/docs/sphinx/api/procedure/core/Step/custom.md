<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 50da7d2313db -->
# `pluss::ai::Step::custom`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
bool custom() const noexcept;
```

## What It Does

Returns `persist().custom` -- whether this step's persistence goes
through the [`registry`](../../registry.md)'s custom-contract mechanism
rather than a built-in adapter's own fixed save/load logic. Set
automatically by the `step(name, options, CustomPersist, runner)`
overload; every built-in adapter ([`lymph`](../../lymph.md)/
[`ice`](../../ice.md)/[`nlp`](../../nlp.md)/[`SinkSpec`](../SinkSpec.md))
leaves it `false`.

## Parameters

None.

## Returns

`true` or `false`.

## Errors

None.

## Practical Examples

### Example 1

A sink step's built-in persistence is not "custom".

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "custom=" << s.custom() << "\n";
  return 0;
}
```

Output

```text
custom=0
```

### Example 2

A hand-attached [`StepPersistSpec`](../StepPersistSpec.md) with
`custom = true` set explicitly -- the same flag the
[`registry`](../../registry.md)'s `CustomPersist` overload sets for a
caller.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.key = "my_key";
  persist.custom = true;
  const ai::Step s = ai::procedure::step("custom", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "custom=" << s.custom() << "\n";
  return 0;
}
```

Output

```text
custom=1
```

### Example 3

A plain callable step with no persist spec reads `false`.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "custom=" << s.custom() << "\n";
  return 0;
}
```

Output

```text
custom=0
```

## See Also

- [`registry`](../../registry.md) -- `CustomContract`/`CustomPersist`,
  what sets this flag in practice.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="component_schema_version.html">
    <span class="label">Previous</span>
    <span class="title">component_schema_version</span>
  </a>
  <a class="next-link" href="frozen.html">
    <span class="label">Next</span>
    <span class="title">frozen</span>
  </a>
</div>
<!-- apiref:nav:end -->
