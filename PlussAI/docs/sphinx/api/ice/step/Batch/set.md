<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 35a05c6b0f60 -->
# `pluss::ai::ice::Batch::set`

[Back to `pluss::ai::ice::Batch`](../Batch.md)

## Signature

```cpp
void pluss::ai::ice::Batch::set(std::string name, pluss::Series values);
```

## What It Does

Sets a named field's value, replacing it in place if `name` already
exists rather than appending a second entry with the same name.
[`set_inputs`](set_inputs.md) is implemented as `set("inputs", values)`
-- there is nothing special about the name `"inputs"` here beyond that
one convenience wrapper. `"targets"` is not special-cased either: setting
a field literally named `"targets"` through `set` adds it to the plain
field list, completely independent of [`set_targets`](set_targets.md)'s
own separately-tracked value.

## Parameters

`std::string name`
: The field name. Replaces an existing field of the same name, or
  appends a new one.

`Series values`
: The field's value.

## Returns

`void`.

## Errors

None from `set` itself.

## Practical Examples

Two examples: setting a new field, and replacing an existing one by
calling `set` again with the same name.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set("mask", pluss::make_series({1.0f, 1.0f}, {2}));

  std::cout << "fields().size() = " << batch.fields().size() << "\n";

  return 0;
}
```

Output

```text
fields().size() = 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set("mask", pluss::make_series({1.0f, 1.0f}, {2}));
  batch.set("mask", pluss::make_series({0.0f, 0.0f}, {2}));

  std::cout << "fields().size() after re-set = " << batch.fields().size() << "\n";
  std::cout << "field(\"mask\") after re-set = " << batch.field("mask") << "\n";

  return 0;
}
```

Output

```text
fields().size() after re-set = 1
field("mask") after re-set = pluss::Series(shape=[2], strides=[1], dtype=float32, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
```

### Example 3

`set` is not limited to `"inputs"`/`"targets"` -- any field name works,
read back via [`fields`](fields.md) or [`field`](field.md).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set("weights", pluss::make_series({0.5f, 0.5f}, {2}));

  std::cout << "fields()[0].name = " << batch.fields()[0].name << "\n";

  return 0;
}
```

Output

```text
fields()[0].name = weights
```

## See Also

- [`field`](field.md) / [`has`](has.md) -- read a field back.
- [`set_inputs`](set_inputs.md) / [`set_targets`](set_targets.md) -- the
  two typed convenience wrappers.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="inputs.html">
    <span class="label">Previous</span>
    <span class="title">inputs</span>
  </a>
  <a class="next-link" href="set_inputs.html">
    <span class="label">Next</span>
    <span class="title">set_inputs</span>
  </a>
</div>
<!-- apiref:nav:end -->
