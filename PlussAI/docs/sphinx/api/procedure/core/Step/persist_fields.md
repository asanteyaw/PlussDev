<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 15676bfe9693 -->
# `pluss::ai::Step::persist_fields`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
const std::vector<std::pair<std::string, std::string>>& persist_fields() const noexcept;
```

## What It Does

Returns `persist().fields` -- a flat `key, value` list of extra metadata
written into a saved bundle's audit records alongside the component
artifact itself (hyperparameters, training-time flags, or anything else
worth recording for later inspection without needing to reload the
component). Every value is a plain string; callers format whatever they
need into that form before attaching a
[`StepPersistSpec`](../StepPersistSpec.md).

## Parameters

None.

## Returns

A `const std::vector<std::pair<std::string, std::string>>&`, empty for a
step with no persist spec attached.

## Errors

None.

## Practical Examples

### Example 1

A sink step's own persist spec records its target under one field.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "field_count=" << s.persist_fields().size() << "\n";
  return 0;
}
```

Output

```text
field_count=1
```

### Example 2

A hand-attached [`StepPersistSpec`](../StepPersistSpec.md) with two
fields of its own.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.key = "my_key";
  persist.fields = {{"alpha", "1"}, {"beta", "2"}};
  const ai::Step s = ai::procedure::step("custom", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "field_count=" << s.persist_fields().size() << "\n";
  return 0;
}
```

Output

```text
field_count=2
```

### Example 3

A plain callable step with no persist spec has no fields at all.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "field_count=" << s.persist_fields().size() << "\n";
  return 0;
}
```

Output

```text
field_count=0
```

## See Also

- [`StepPersistSpec`](../StepPersistSpec.md) -- the full struct.
- [`persist`](../../persist.md) -- where these fields land in the saved
  bundle's audit directory.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="persist.html">
    <span class="label">Previous</span>
    <span class="title">persist</span>
  </a>
  <a class="next-link" href="persist_key.html">
    <span class="label">Next</span>
    <span class="title">persist_key</span>
  </a>
</div>
<!-- apiref:nav:end -->
