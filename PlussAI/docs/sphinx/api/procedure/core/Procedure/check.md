<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cfbe7d063d97 -->
# `pluss::ai::Procedure::check`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
void check() const;
```

## What It Does

Validates the graph exactly like [`check_message`](check_message.md),
but throws the first problem found as a `pluss::Error` instead of
returning it. This is the call `Procedure`'s constructor makes
automatically, unless
[`ProcedureOptions::validate_on_make`](../ProcedureOptions.md) is set
`false` -- most callers never call `check()` directly, since by the time
they hold a `Procedure` value it has usually already passed.

## Parameters

None.

## Returns

Nothing (`void`).

## Errors

- Throws with the same message [`check_message`](check_message.md) would
  return, for the same reasons (cycles, duplicate names, dangling links,
  steps with no capabilities, and so on).

## Practical Examples

### Example 1

A valid graph's `check()` call simply returns.

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

  proc.check();
  std::cout << "ok=1\n";
  return 0;
}
```

Output

```text
ok=1
```

### Example 2

An invalid graph (constructed with `validate_on_make = false` to reach
this state at all) throws when `check()` is called explicitly.

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

  bool threw = false;
  try {
    cyclic.check();
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "threw=" << threw << "\n";
  return 0;
}
```

Output

```text
threw=1
```

### Example 3

The same invalid graph throws immediately from `make()` itself when
`validate_on_make` is left at its default `true` -- the construction
call never completes.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  bool threw = false;
  try {
    const ai::Procedure cyclic = ai::procedure::make({
        ai::procedure::step("a"), ai::procedure::step("b"),
    }, {
        ai::procedure::link("a", "b"), ai::procedure::link("b", "a"),
    });
    (void)cyclic;
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "threw=" << threw << "\n";
  return 0;
}
```

Output

```text
threw=1
```

## See Also

- [`check_message`](check_message.md) -- the non-throwing form.
- [`ProcedureOptions`](../ProcedureOptions.md) -- `validate_on_make`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="check_message.html">
    <span class="label">Next</span>
    <span class="title">check_message</span>
  </a>
</div>
<!-- apiref:nav:end -->
