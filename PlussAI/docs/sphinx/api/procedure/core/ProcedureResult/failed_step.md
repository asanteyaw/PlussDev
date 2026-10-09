<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bc3757fc55e1 -->

# `pluss::ai::ProcedureResult::failed_step`

[Back to `pluss::ai::ProcedureResult`](../ProcedureResult.md)

## Signature

```cpp
const std::string& failed_step() const noexcept;
```

## What It Does

Names the step that stopped the run, or is empty when the run completed
fully. [`ok()`](ok.md) is defined directly in terms of this being empty
-- only reachable as non-empty when
[`ProcedureOptions::fail_fast`](../ProcedureOptions.md) is `false`, since
with the default `true` the step's own exception propagates out of
`run()` instead of being recorded here.

## Parameters

None.

## Returns

A `const std::string&` -- the failed step's name, or empty.

## Errors

None.

## Practical Examples

### Example 1

A step whose declared input was never seeded fails, and
`fail_fast = false` lets the run return normally with that step named.

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
  std::cout << "failed_step=" << out.failed_step() << "\n";
  return 0;
}
```

Output

```text
failed_step=needs_signal
```

### Example 2

A successful run's `failed_step()` is empty.

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
  std::cout << "failed_step=[" << out.failed_step() << "]\n";
  return 0;
}
```

Output

```text
failed_step=[]
```

### Example 3

With two independent failing steps, the one named is whichever runs
(and fails) first in topological order.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions options{};
  options.fail_fast = false;
  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("a", "missing_a", "out_a", [](const pluss::Series& s) { return s; }),
      ai::procedure::step("b", "missing_b", "out_b", [](const pluss::Series& s) { return s; }),
  }, {}, options);

  const ai::ProcedureResult out = proc.run();
  std::cout << "failed_step=" << out.failed_step() << "\n";
  return 0;
}
```

Output

```text
failed_step=a
```

## See Also

- [`ok`](ok.md) -- the boolean form of this same question.
- [`warnings`](warnings.md) -- the exception message that goes with
  this step's failure.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="audit.html">
    <span class="label">Previous</span>
    <span class="title">audit</span>
  </a>
  <a class="next-link" href="get.html">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>
<!-- apiref:nav:end -->
