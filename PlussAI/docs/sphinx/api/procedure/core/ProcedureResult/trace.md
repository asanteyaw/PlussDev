<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d2d228623055 -->

# `pluss::ai::ProcedureResult::trace`

[Back to `pluss::ai::ProcedureResult`](../ProcedureResult.md)

## Signature

```cpp
const std::vector<std::string>& trace() const noexcept;
```

## What It Does

Returns a `"ran:<name>"` entry per step that completed, in execution
order -- always collected, regardless of
[`RunOptions::audit`](../RunOptions.md). This is the cheap log-line form
of run history; [`audit`](audit.md) is the fuller, structured form.

## Parameters

None.

## Returns

A `const std::vector<std::string>&`.

## Errors

None.

## Practical Examples

### Example 1

A two-step run's trace, in order.

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
  std::cout << "first=" << out.trace().front() << "\n";
  return 0;
}
```

Output

```text
first=ran:scale
```

### Example 2

`trace().size()` matches the number of steps that actually ran.

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
  std::cout << "count=" << out.trace().size() << "\n";
  return 0;
}
```

Output

```text
count=2
```

### Example 3

A step that fails before completing is never added to `trace()` --
with `fail_fast = false`, a failing single-step run leaves `trace()`
empty.

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
  std::cout << "count=" << out.trace().size() << "\n";
  return 0;
}
```

Output

```text
count=0
```

## See Also

- [`audit`](audit.md) -- the fuller, structured alternative.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ok.html">
    <span class="label">Previous</span>
    <span class="title">ok</span>
  </a>
  <a class="next-link" href="warnings.html">
    <span class="label">Next</span>
    <span class="title">warnings</span>
  </a>
</div>
<!-- apiref:nav:end -->
