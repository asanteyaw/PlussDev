<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 72bad54b444f -->
# `pluss::ai::ice::Batch::has`

[Back to `pluss::ai::ice::Batch`](../Batch.md)

## Signature

```cpp
bool pluss::ai::ice::Batch::has(std::string_view name) const noexcept;
```

## What It Does

Reports whether a field named `name` has been set via
[`set`](set.md)/[`set_inputs`](set_inputs.md) -- `"inputs"` counts the
same as any other name here, since `set_inputs` is implemented in terms
of `set`. `targets` is tracked separately (see
[`has_targets`](has_targets.md)) and never counts as a `has` match, even
under the name `"targets"`.

## Parameters

`std::string_view name`
: The field name to look up.

## Returns

`bool` -- `true` if a field with that exact name has been set.

## Errors

None. `has` is `noexcept` and never throws, regardless of `name`.

## Practical Examples

Two examples: checking a field that was set, and one that was not.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set("mask", pluss::make_series({1.0f, 1.0f, 0.0f}, {3}));

  std::cout << "has(\"mask\") = " << batch.has("mask") << "\n";

  return 0;
}
```

Output

```text
has("mask") = 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set("mask", pluss::make_series({1.0f, 1.0f, 0.0f}, {3}));

  std::cout << "has(\"missing\") = " << batch.has("missing") << "\n";

  return 0;
}
```

Output

```text
has("missing") = 0
```

### Example 3

`has` on a batch with `inputs`/`targets`/an extra field all set together
-- `has("inputs")` and `has("targets")` behave differently: `targets` is
tracked separately and never counts as a `has` match.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f}, {1}));
  batch.set_targets(pluss::make_series({2.0f}, {1}));
  batch.set("mask", pluss::make_series({1.0f}, {1}));

  std::cout << "has(\"inputs\") = " << batch.has("inputs") << "\n";
  std::cout << "has(\"mask\") = " << batch.has("mask") << "\n";
  std::cout << "has(\"targets\") = " << batch.has("targets") << "\n";

  return 0;
}
```

Output

```text
has("inputs") = 1
has("mask") = 1
has("targets") = 0
```

## See Also

- [`field`](field.md) -- reads the field back once `has` confirms it
  exists.
- [`has_inputs`](has_inputs.md) / [`has_targets`](has_targets.md) -- the
  same check for the two specially-tracked fields.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fields.html">
    <span class="label">Previous</span>
    <span class="title">fields</span>
  </a>
  <a class="next-link" href="has_inputs.html">
    <span class="label">Next</span>
    <span class="title">has_inputs</span>
  </a>
</div>
<!-- apiref:nav:end -->
