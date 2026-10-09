<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c438d9fb9fbb -->

# `pluss::ai::ProcedureResult::warnings`

[Back to `pluss::ai::ProcedureResult`](../ProcedureResult.md)

## Signature

```cpp
const std::vector<std::string>& warnings() const noexcept;
```

## What It Does

Returns the non-fatal messages collected during the run -- populated
only under [`ProcedureOptions::fail_fast`](../ProcedureOptions.md)
`= false`, where a step's exception message is appended here instead of
propagating out of [`run`](../Procedure/run.md). A successful run (or any
run with `fail_fast` at its default `true`) always has an empty
`warnings()`.

## Parameters

None.

## Returns

A `const std::vector<std::string>&`.

## Errors

None.

## Practical Examples

### Example 1

A successful run has no warnings.

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
  std::cout << "warnings=" << out.warnings().size() << "\n";
  return 0;
}
```

Output

```text
warnings=0
```

### Example 2

A failed step under `fail_fast = false` adds exactly one warning.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions options{};
  options.fail_fast = false;
  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("needs_signal", "signal", "out", [](const pluss::Series& s) { return s; }),
  }, {}, options);

  const ai::ProcedureResult out = proc.run();
  std::cout << "warnings=" << out.warnings().size() << "\n";
  return 0;
}
```

Output

```text
warnings=1
```

### Example 3

The warning is the failing step's own exception message, not a generic
string.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions options{};
  options.fail_fast = false;
  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("needs_signal", "signal", "out", [](const pluss::Series& s) { return s; }),
  }, {}, options);

  const ai::ProcedureResult out = proc.run();
  std::cout << "non_empty=" << !out.warnings().front().empty() << "\n";
  return 0;
}
```

Output

```text
non_empty=1
```

## See Also

- [`failed_step`](failed_step.md) -- which step this warning came from.
- [`ProcedureOptions::fail_fast`](../ProcedureOptions.md) -- what makes
  this list reachable as non-empty at all.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="trace.html">
    <span class="label">Previous</span>
    <span class="title">trace</span>
  </a>
</div>
<!-- apiref:nav:end -->
