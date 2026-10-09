<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 988cfb0e735a -->

# `pluss::ai::ProcedureResult::get`

[Back to `pluss::ai::ProcedureResult`](../ProcedureResult.md)

## Signature

```cpp
template <typename T>
const T& get(std::string_view name) const;
```

## What It Does

Finds the [`Artifact`](../Artifact.md) named `name` in
[`artifacts()`](artifacts.md) and returns its value as `T`, throwing if
no artifact has that name (regardless of type). This is the normal way a
caller reads a run's result, since both an artifact's presence and its
type are checked in one call rather than two.

## Parameters

`std::string_view name`
: The artifact name to look up (an input's own name, or a step's
  declared or default output name).
`T` (template parameter)
: The expected value type.

## Returns

A `const T&` into the result's own storage.

## Errors

- Throws if no artifact named `name` exists.
- Throws (via [`Artifact::get`](../Artifact/get.md)) if the artifact
  exists but does not hold a `T`.

## Practical Examples

### Example 1

Reading the final step's output by its declared name.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) { return s.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "elements=" << out.get<pluss::Series>("score").numel() << "\n";
  return 0;
}
```

Output

```text
elements=3
```

### Example 2

Asking for a name that was never produced throws.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0}, {1}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) { return s.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  bool threw = false;
  try {
    out.get<pluss::Series>("missing");
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

The seeded input itself is a readable artifact, under its own name.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0}, {1}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) { return s.mul(10.0); }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) { return s.add(2.0); }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "elements=" << out.get<pluss::Series>("x").numel() << "\n";
  return 0;
}
```

Output

```text
elements=1
```

## See Also

- [`artifacts`](artifacts.md) -- walking every produced value instead
  of looking one up by name.
- [`Artifact::get`](../Artifact/get.md) -- the type-check this builds
  on.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="failed_step.html">
    <span class="label">Previous</span>
    <span class="title">failed_step</span>
  </a>
  <a class="next-link" href="metrics.html">
    <span class="label">Next</span>
    <span class="title">metrics</span>
  </a>
</div>
<!-- apiref:nav:end -->
