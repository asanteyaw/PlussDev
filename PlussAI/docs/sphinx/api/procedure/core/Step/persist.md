<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7020727a9ecc -->
# `pluss::ai::Step::persist`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
const StepPersistSpec& persist() const noexcept;
```

## What It Does

Returns the step's full [`StepPersistSpec`](../StepPersistSpec.md).
Every other persistence accessor (`reloadable`/`frozen`/`custom`/
`persist_key`/`persist_schema_version`/`component_schema_version`/
`persist_fields`/`can_save_component`) is a thin forward over one field
of this struct; `persist()` is the escape hatch for reading more than one
field at once, or a field none of those expose.

## Parameters

None.

## Returns

A `const StepPersistSpec&` -- default-constructed (all fields at their
defaults, `key` empty) for a step that never attached one.

## Errors

None.

## Practical Examples

### Example 1

A sink step's persist spec has a fixed, built-in key.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "key=" << s.persist().key << "\n";
  return 0;
}
```

Output

```text
key=procedure::Sink
```

### Example 2

A plain callable step with no persistence spec attached has an empty
key.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "key=[" << s.persist().key << "]\n";
  return 0;
}
```

Output

```text
key=[]
```

### Example 3

A hand-attached [`StepPersistSpec`](../StepPersistSpec.md) (via the
`step(name, options, StepPersistSpec, runner)` overload) carries whatever
key the caller gave it.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.key = "my_key";
  const ai::Step s = ai::procedure::step("custom", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "key=" << s.persist().key << "\n";
  return 0;
}
```

Output

```text
key=my_key
```

## See Also

- [`StepPersistSpec`](../StepPersistSpec.md) -- every field's meaning.
- [`persist_key`](persist_key.md) -- the key alone.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="outputs.html">
    <span class="label">Previous</span>
    <span class="title">outputs</span>
  </a>
  <a class="next-link" href="persist_fields.html">
    <span class="label">Next</span>
    <span class="title">persist_fields</span>
  </a>
</div>
<!-- apiref:nav:end -->
