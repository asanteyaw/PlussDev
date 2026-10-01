<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1e3937657897 -->
# `pluss::ai::ice::Batch::field`

[Back to `pluss::ai::ice::Batch`](../Batch.md)

## Signature

```cpp
const pluss::Series& pluss::ai::ice::Batch::field(std::string_view name) const;
pluss::Series& pluss::ai::ice::Batch::field(std::string_view name);
```

## What It Does

Looks up a field by name -- the general accessor underneath
[`inputs`](inputs.md)/[`set_inputs`](set_inputs.md) and every other
named field a caller has [`set`](set.md). The non-`const` overload
returns a mutable reference, so a field can be updated in place without
going through `set` again. Throws if `name` was never set; check
[`has`](has.md) first when a field's presence isn't already known.

## Parameters

`std::string_view name`
: The field name to look up.

## Returns

`const Series&` (on a `const Batch`) or `Series&` (otherwise) -- a
reference to that field's value.

## Errors

- `name` was never set via `set`/`set_inputs` -- throws.

## Practical Examples

Two examples: reading an existing field, and the error from a missing
one.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set("mask", pluss::make_series({1.0f, 1.0f, 0.0f}, {3}));

  std::cout << "field(\"mask\"):\n" << batch.field("mask") << "\n";

  return 0;
}
```

Output

```text
field("mask"):
pluss::Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 0]
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  try {
    const ice::Batch& const_batch = batch;
    const_batch.field("missing");
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "threw: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
threw: pluss::ai::ice::Batch::field: no field named 'missing'.
```

### Example 3

The non-`const` overload returns a mutable reference, so a field can be
edited in place without calling `set` again.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set("mask", pluss::make_series({1.0f, 1.0f}, {2}));
  batch.field("mask") = pluss::make_series({0.0f, 0.0f}, {2});

  std::cout << "field(\"mask\") after in-place edit:\n" << batch.field("mask") << "\n";

  return 0;
}
```

Output

```text
field("mask") after in-place edit:
pluss::Series(shape=[2], strides=[1], dtype=float32, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
```

## See Also

- [`has`](has.md) -- checks presence without throwing.
- [`fields`](fields.md) -- every field at once, as a list.
- [`set`](set.md) -- sets or replaces a field.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="fields.html">
    <span class="label">Next</span>
    <span class="title">fields</span>
  </a>
</div>
<!-- apiref:nav:end -->
