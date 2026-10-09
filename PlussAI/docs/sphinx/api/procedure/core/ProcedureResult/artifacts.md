<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 732fe5de62f2 -->

# `pluss::ai::ProcedureResult::artifacts`

[Back to `pluss::ai::ProcedureResult`](../ProcedureResult.md)

## Signature

```cpp
const std::vector<Artifact>& artifacts() const noexcept;
```

## What It Does

Returns every [`Artifact`](../Artifact.md) added to the run's pool, in
the order each was first produced -- the seeded `inputs` first, then
each step's own output as it runs. [`get`](get.md) is the common way to
pick one out by name; `artifacts()` is for code that wants to walk all of
them (logging every produced value, or persisting the whole set).

## Parameters

None.

## Returns

A `const std::vector<Artifact>&`.

## Errors

None.

## Practical Examples

### Example 1

A two-step run over one seeded input produces three artifacts in total:
the input itself, plus each step's own output.

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
  std::cout << "count=" << out.artifacts().size() << "\n";
  return 0;
}
```

Output

```text
count=3
```

### Example 2

A sink step with no input returns one empty, named artifact.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink")),
  });

  const ai::ProcedureResult out = proc.run();
  std::cout << "front_empty=" << out.artifacts().front().empty() << "\n";
  return 0;
}
```

Output

```text
front_empty=1
```

### Example 3

Artifacts appear in production order -- the last entry is the final
step's own output.

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
  std::cout << "last=" << out.artifacts().back().name() << "\n";
  return 0;
}
```

Output

```text
last=score
```

## See Also

- [`get`](get.md) -- fetch one artifact by name, type-checked.
- [`metrics`](metrics.md) -- the sibling list for step-reported metrics.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="audit.html">
    <span class="label">Next</span>
    <span class="title">audit</span>
  </a>
</div>
<!-- apiref:nav:end -->
