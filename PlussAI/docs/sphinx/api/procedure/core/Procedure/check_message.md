<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d41c0cb396b5 -->
# `pluss::ai::Procedure::check_message`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
std::optional<std::string> check_message() const;
```

## What It Does

Validates the procedure's graph -- worker count, step-name/port
uniqueness, every step declaring at least one
[`StepCapability`](../StepOptions.md), every link's endpoints and named
ports existing, and the whole graph being acyclic -- and returns the
*first* problem found as a message, or `std::nullopt` if the graph is
valid. This is the non-throwing form [`check`](check.md) is built on; use
it when a caller wants to decide what to do with an invalid graph rather
than catching an exception.

## Parameters

None.

## Returns

`std::nullopt` if the graph is valid, otherwise a `std::string` describing
the first problem found.

## Errors

None -- `check_message` itself never throws; it reports problems instead
of throwing on them.

## Practical Examples

### Example 1

A valid graph reports `std::nullopt`.

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

  std::cout << "has_value=" << proc.check_message().has_value() << "\n";
  return 0;
}
```

Output

```text
has_value=0
```

### Example 2

A cyclic graph (only reachable with
[`ProcedureOptions::validate_on_make`](../ProcedureOptions.md) set
`false`, since `make()` would otherwise throw first) reports a problem.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions no_validate{};
  no_validate.validate_on_make = false;
  const ai::Procedure cyclic = ai::procedure::make({
      ai::procedure::step("a"), ai::procedure::step("b"),
  }, {
      ai::procedure::link("a", "b"), ai::procedure::link("b", "a"),
  }, no_validate);

  std::cout << "has_value=" << cyclic.check_message().has_value() << "\n";
  return 0;
}
```

Output

```text
has_value=1
```

### Example 3

The message names the specific problem -- here, a duplicate step name.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions no_validate{};
  no_validate.validate_on_make = false;
  const ai::Procedure dup = ai::procedure::make({
      ai::procedure::step("dup"), ai::procedure::step("dup"),
  }, {}, no_validate);

  std::cout << *dup.check_message() << "\n";
  return 0;
}
```

Output

```text
pluss::ai::Procedure::check: duplicate step name "dup".
```

## See Also

- [`check`](check.md) -- the throwing form, used automatically on
  construction unless `validate_on_make` is disabled.
- [`ProcedureOptions`](../ProcedureOptions.md) -- `validate_on_make`'s own
  page.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="check.html">
    <span class="label">Previous</span>
    <span class="title">check</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
<!-- apiref:nav:end -->
