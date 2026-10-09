<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1ee9e151ee18 -->
# `pluss::ai::Procedure::links`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
const std::vector<Link>& links() const noexcept;
```

## What It Does

Returns every [`Link`](../Link.md) in the procedure, in declaration order.
A step with no incoming or outgoing links simply has none of its own
entries here -- `links()` lists edges, not nodes.

## Parameters

None.

## Returns

A `const std::vector<Link>&` -- read-only, insertion-ordered.

## Errors

None -- always succeeds; a single-step procedure returns an empty vector.

## Practical Examples

### Example 1

A one-link procedure's `links().size()` is `1`.

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

  std::cout << "link_count=" << proc.links().size() << "\n";
  return 0;
}
```

Output

```text
link_count=1
```

### Example 2

Each `Link` exposes its own `from`/`to` fields directly.

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

  std::cout << "from=" << proc.links().front().from << " to=" << proc.links().front().to << "\n";
  return 0;
}
```

Output

```text
from=scale to=shift
```

### Example 3

A single-step procedure genuinely has zero links -- there is nothing to
connect.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({ai::procedure::step("solo")});
  std::cout << "link_count=" << proc.links().size() << "\n";
  return 0;
}
```

Output

```text
link_count=0
```

## See Also

- [`steps`](steps.md) -- the companion node list.
- [`link_count`](link_count.md) -- just the count.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="link_count.html">
    <span class="label">Previous</span>
    <span class="title">link_count</span>
  </a>
  <a class="next-link" href="names.html">
    <span class="label">Next</span>
    <span class="title">names</span>
  </a>
</div>
<!-- apiref:nav:end -->
