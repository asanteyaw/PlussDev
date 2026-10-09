<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ce24bb9b3ebf -->
# `pluss::ai::Procedure::names`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
std::vector<std::string> names() const;
```

## What It Does

Returns every step's name, in the same insertion order as
[`steps()`](steps.md) -- a convenience for callers who only need the
names (for logging, for building a lookup set, or for driving
[`has`](has.md)/[`get`](get.md) by name) without walking full `Step`
objects.

## Parameters

None.

## Returns

A fresh `std::vector<std::string>`, one entry per step, in declaration
order.

## Errors

None.

## Practical Examples

### Example 1

The first name matches the first step passed to `make`.

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

  std::cout << "first=" << proc.names().front() << "\n";
  return 0;
}
```

Output

```text
first=scale
```

### Example 2

`names().size()` matches [`step_count`](step_count.md).

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

  std::cout << "count=" << proc.names().size() << "\n";
  return 0;
}
```

Output

```text
count=2
```

### Example 3

Insertion order survives for a longer chain, regardless of link order.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>
#include <string>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("a"), ai::procedure::step("b"), ai::procedure::step("c"),
  }, {
      ai::procedure::link("a", "b"), ai::procedure::link("b", "c"),
  });

  std::string order;
  for (const std::string& n : proc.names()) {
    if (!order.empty()) order += ",";
    order += n;
  }
  std::cout << "order=" << order << "\n";
  return 0;
}
```

Output

```text
order=a,b,c
```

## See Also

- [`steps`](steps.md) -- the full `Step` objects this names.
- [`has`](has.md) / [`get`](get.md) -- look up one step by name.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="links.html">
    <span class="label">Previous</span>
    <span class="title">links</span>
  </a>
  <a class="next-link" href="options.html">
    <span class="label">Next</span>
    <span class="title">options</span>
  </a>
</div>
<!-- apiref:nav:end -->
