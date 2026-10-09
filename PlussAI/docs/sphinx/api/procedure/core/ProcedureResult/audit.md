<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 41f0bf5adcb3 -->

# `pluss::ai::ProcedureResult::audit`

[Back to `pluss::ai::ProcedureResult`](../ProcedureResult.md)

## Signature

```cpp
const std::vector<StepTrace>& audit() const noexcept;
```

## What It Does

Returns one [`StepTrace`](../StepTrace.md) per step that ran, in
execution order -- each input's and the output's
[`ArtifactTrace`](../ArtifactTrace.md), elapsed microseconds, and whether
row count changed. Populated only when
[`RunOptions::audit`](../RunOptions.md) was `true` (the default); with it
`false`, this stays empty regardless of how many steps ran.

## Parameters

None.

## Returns

A `const std::vector<StepTrace>&`.

## Errors

None.

## Practical Examples

### Example 1

A two-step run produces two audit records, with audit on by default.

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
  std::cout << "count=" << out.audit().size() << "\n";
  return 0;
}
```

Output

```text
count=2
```

### Example 2

With `RunOptions::audit = false`, `audit()` stays empty even though the
run itself still completes and `trace()` still records it.

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

  ai::RunOptions opts{};
  opts.audit = false;
  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)}, opts);
  std::cout << "count=" << out.audit().size() << "\n";
  return 0;
}
```

Output

```text
count=0
```

### Example 3

Each `StepTrace`'s own `step` field names which step it describes.

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
  std::cout << "first_step=" << out.audit().front().step << "\n";
  return 0;
}
```

Output

```text
first_step=scale
```

## See Also

- [`StepTrace`](../StepTrace.md) -- every field's meaning.
- [`trace`](trace.md) -- the cheaper, always-on alternative.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="artifacts.html">
    <span class="label">Previous</span>
    <span class="title">artifacts</span>
  </a>
  <a class="next-link" href="failed_step.html">
    <span class="label">Next</span>
    <span class="title">failed_step</span>
  </a>
</div>
<!-- apiref:nav:end -->
