<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b2197375420a -->
# `pluss::ai::Procedure::has`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
bool has(std::string_view name) const noexcept;
```

## What It Does

Reports whether a step named `name` exists in the procedure. Use this to
check membership before calling [`get`](get.md), which throws instead of
returning a sentinel when the name is absent.

## Parameters

`std::string_view name`
: The step name to look for.

## Returns

`true` if some step's `name()` equals `name`, `false` otherwise.

## Errors

None -- never throws, regardless of `name`.

## Practical Examples

### Example 1

A name that was actually declared is found.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) { return s.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  std::cout << "has_scale=" << proc.has("scale") << "\n";
  return 0;
}
```

Output

```text
has_scale=1
```

### Example 2

A name that was never declared returns `false`, not an error.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) { return s.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  std::cout << "has_missing=" << proc.has("nonexistent") << "\n";
  return 0;
}
```

Output

```text
has_missing=0
```

### Example 3

Every declared step is found, not only the first.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) { return s.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  std::cout << "has_shift=" << proc.has("shift") << "\n";
  return 0;
}
```

Output

```text
has_shift=1
```

## See Also

- [`get`](get.md) -- fetch the `Step` itself, throwing if absent.
- [`names`](names.md) -- every declared name at once.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="link_count.html">
    <span class="label">Next</span>
    <span class="title">link_count</span>
  </a>
</div>
<!-- apiref:nav:end -->
