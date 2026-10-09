<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8645ca241506 -->
# `pluss::ai::Procedure::link_count`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
std::int64_t link_count() const noexcept;
```

## What It Does

Returns `links().size()` as a signed `std::int64_t`, the edge-count
counterpart to [`step_count`](step_count.md).

## Parameters

None.

## Returns

A non-negative `std::int64_t`.

## Errors

None.

## Practical Examples

### Example 1

A two-step, one-link procedure reports `link_count() == 1`.

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

  std::cout << "link_count=" << proc.link_count() << "\n";
  return 0;
}
```

Output

```text
link_count=1
```

### Example 2

A three-step chain has two links.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("a"), ai::procedure::step("b"), ai::procedure::step("c"),
  }, {
      ai::procedure::link("a", "b"), ai::procedure::link("b", "c"),
  });

  std::cout << "link_count=" << proc.link_count() << "\n";
  return 0;
}
```

Output

```text
link_count=2
```

### Example 3

A single, unconnected step has zero links.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({ai::procedure::step("solo")});
  std::cout << "link_count=" << proc.link_count() << "\n";
  return 0;
}
```

Output

```text
link_count=0
```

## See Also

- [`links`](links.md) -- the full vector this counts.
- [`step_count`](step_count.md) -- the node-side equivalent.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="links.html">
    <span class="label">Next</span>
    <span class="title">links</span>
  </a>
</div>
<!-- apiref:nav:end -->
