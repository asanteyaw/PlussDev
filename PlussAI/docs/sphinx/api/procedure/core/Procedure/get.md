<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ad96f748ce3f -->
# `pluss::ai::Procedure::get`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
const Step& get(std::string_view name) const;
```

## What It Does

Looks up a step by name and returns it. Unlike [`has`](has.md), an absent
name is treated as a programming error, not a query result -- check
`has(name)` first if the name is not known to exist.

## Parameters

`std::string_view name`
: The step name to look up.

## Returns

A `const Step&` into the procedure's own storage.

## Errors

- Throws if no step named `name` exists.

## Practical Examples

### Example 1

Fetching a step by name gives back the same `Step` [`steps()`](steps.md)
holds.

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

  std::cout << "name=" << proc.get("scale").name() << "\n";
  return 0;
}
```

Output

```text
name=scale
```

### Example 2

`get` returning a full `Step` means every `Step` accessor, like
[`has`](../Step/has.md), is immediately available on the result.

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

  std::cout << "has_sink=" << proc.get("scale").has(ai::StepCapability::Sink) << "\n";
  return 0;
}
```

Output

```text
has_sink=0
```

### Example 3

An unknown name throws rather than returning a default `Step`.

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

  bool threw = false;
  try {
    proc.get("missing");
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

- [`has`](has.md) -- check existence without risking a throw.
- [`steps`](steps.md) -- every step, by position instead of by name.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
<!-- apiref:nav:end -->
