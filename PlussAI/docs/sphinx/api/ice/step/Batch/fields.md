<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1273f88c8ed6 -->
# `pluss::ai::ice::Batch::fields`

[Back to `pluss::ai::ice::Batch`](../Batch.md)

## Signature

```cpp
const std::vector<pluss::ai::ice::BatchField>& pluss::ai::ice::Batch::fields() const noexcept;
```

## What It Does

Returns every field set via [`set`](set.md)/[`set_inputs`](set_inputs.md),
in insertion order -- `targets` is excluded (it is tracked separately,
see [`targets`](targets.md)). Each entry is a plain `BatchField { std::string
name; Series values; }`. Useful for iterating over every field a
particular `Model` subclass's step override expects, without knowing
every name in advance.

## Parameters

None.

## Returns

`const std::vector<BatchField>&` -- every non-`targets` field, in the
order it was first set.

## Errors

None. `fields` is `noexcept` and has no preconditions.

## Practical Examples

One example: setting several fields and iterating over all of them.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f, 2.0f, 3.0f}, {3}));
  batch.set("mask", pluss::make_series({1.0f, 1.0f, 0.0f}, {3}));
  batch.set_targets(pluss::make_series({0.0f, 1.0f, 0.0f}, {3}));

  std::cout << "fields().size() = " << batch.fields().size() << "\n";
  for (const auto& field : batch.fields()) {
    std::cout << "  '" << field.name << "' = " << field.values << "\n";
  }

  return 0;
}
```

Output

```text
fields().size() = 2
  'inputs' = pluss::Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
  'mask' = pluss::Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 0]
```

### Example 2

A freshly constructed `Batch` has no fields at all.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  std::cout << "fields().size() = " << batch.fields().size() << "\n";

  return 0;
}
```

Output

```text
fields().size() = 0
```

### Example 3

`targets` never appears in `fields()`, even when set -- it is tracked
separately (see [`targets`](targets.md)).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_targets(pluss::make_series({1.0f}, {1}));

  std::cout << "fields().size() = " << batch.fields().size() << "\n";
  std::cout << "has_targets() = " << batch.has_targets() << "\n";

  return 0;
}
```

Output

```text
fields().size() = 0
has_targets() = 1
```

## See Also

- [`field`](field.md) -- look up one field by name.
- [`targets`](targets.md) -- the separately-tracked field `fields` does
  not include.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="field.html">
    <span class="label">Previous</span>
    <span class="title">field</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
<!-- apiref:nav:end -->
