<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dc80e648776b -->
# `pluss::ai::Procedure::steps`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
const std::vector<Step>& steps() const noexcept;
```

## What It Does

Returns every [`Step`](../Step.md) in the procedure, in the exact order
they were passed to [`make`](../../build/make.md) (and the order
[`run`](run.md) uses as its base topological order, since `Procedure`
runs independent steps in the order they were declared). The reference is
valid for the `Procedure`'s own lifetime.

## Parameters

None.

## Returns

A `const std::vector<Step>&` -- read-only, insertion-ordered.

## Errors

None -- always succeeds, even for an [`empty`](empty.md) procedure (which
returns an empty vector).

## Practical Examples

Three ways `steps()` gets used: counting them, inspecting one by its
position, and walking all of them by name.

### Example 1

A two-step procedure's `steps().size()` matches what it was built with.

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

  std::cout << "step_count=" << proc.steps().size() << "\n";
  return 0;
}
```

Output

```text
step_count=2
```

### Example 2

`steps().front()` is itself a [`Step`](../Step.md), so every `Step`
accessor is available directly.

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

  std::cout << "first_name=" << proc.steps().front().name() << "\n";
  return 0;
}
```

Output

```text
first_name=scale
```

### Example 3

Walking `steps()` to join every name -- the same order
[`names()`](names.md) returns, since both are driven by the same
underlying vector.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <string>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) { return s.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  std::string joined;
  for (const ai::Step& s : proc.steps()) {
    if (!joined.empty()) joined += ",";
    joined += s.name();
  }
  std::cout << "joined=" << joined << "\n";
  return 0;
}
```

Output

```text
joined=scale,shift
```

## See Also

- [`links`](links.md) -- the companion edge list.
- [`names`](names.md) -- just the names, as a `std::vector<std::string>`.
- [`get`](get.md) -- look up one `Step` by name instead of by position.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="step_count.html">
    <span class="label">Previous</span>
    <span class="title">step_count</span>
  </a>
</div>
<!-- apiref:nav:end -->
