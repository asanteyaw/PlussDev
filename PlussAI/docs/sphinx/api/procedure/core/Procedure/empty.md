<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 40ebe76ce4f0 -->
# `pluss::ai::Procedure::empty`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
bool empty() const noexcept;
```

## What It Does

Returns `steps().empty()`. A `Procedure` with at least one step is never
`empty()`, regardless of how many links it has.

## Parameters

None.

## Returns

`true` if the procedure has zero steps, `false` otherwise.

## Errors

None.

## Practical Examples

### Example 1

A normal, non-trivial procedure is never empty.

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

  std::cout << "empty=" << proc.empty() << "\n";
  return 0;
}
```

Output

```text
empty=0
```

### Example 2

A zero-step graph is only reachable at all with
[`ProcedureOptions::validate_on_make`](../ProcedureOptions.md) set
`false`, since [`check`](check.md) otherwise rejects it immediately;
`empty()` still reports it accurately once such a value exists.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions no_validate{};
  no_validate.validate_on_make = false;
  const ai::Procedure proc = ai::procedure::make(std::vector<ai::Step>{}, std::vector<ai::Link>{}, no_validate);

  std::cout << "empty=" << proc.empty() << "\n";
  return 0;
}
```

Output

```text
empty=1
```

### Example 3

A single-step procedure is not empty, even with no links.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({ai::procedure::step("solo")});
  std::cout << "empty=" << proc.empty() << "\n";
  return 0;
}
```

Output

```text
empty=0
```

## See Also

- [`step_count`](step_count.md) -- the same question as a count.
- [`check`](check.md) -- why an empty graph normally never survives
  construction.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="check_message.html">
    <span class="label">Previous</span>
    <span class="title">check_message</span>
  </a>
  <a class="next-link" href="get.html">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>
<!-- apiref:nav:end -->
