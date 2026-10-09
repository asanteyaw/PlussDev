<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 21674138fae9 -->
# `pluss::ai::Step::has`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
bool has(StepCapability capability) const noexcept;
```

## What It Does

Reports whether `capability` is one of the step's declared capabilities,
by testing its bit against [`capability_mask()`](capability_mask.md).

## Parameters

`StepCapability capability`
: The capability to test for.

## Returns

`true` or `false`.

## Errors

None.

## Practical Examples

### Example 1

A `Map`-capability step reports `true` for `Map`.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "has_map=" << s.has(ai::StepCapability::Map) << "\n";
  return 0;
}
```

Output

```text
has_map=1
```

### Example 2

The same step reports `false` for a capability it never declared.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "has_sink=" << s.has(ai::StepCapability::Sink) << "\n";
  return 0;
}
```

Output

```text
has_sink=0
```

### Example 3

A sink step reports `true` for `Sink` specifically, since that is the
one capability `step(name, SinkSpec)` declares.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "has_sink=" << s.has(ai::StepCapability::Sink) << "\n";
  return 0;
}
```

Output

```text
has_sink=1
```

## See Also

- [`capability_mask`](capability_mask.md) -- the whole set at once.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="frozen.html">
    <span class="label">Previous</span>
    <span class="title">frozen</span>
  </a>
  <a class="next-link" href="inputs.html">
    <span class="label">Next</span>
    <span class="title">inputs</span>
  </a>
</div>
<!-- apiref:nav:end -->
