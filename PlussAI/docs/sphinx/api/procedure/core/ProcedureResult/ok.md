<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 67bf4cde4de3 -->

# `pluss::ai::ProcedureResult::ok`

[Back to `pluss::ai::ProcedureResult`](../ProcedureResult.md)

## Signature

```cpp
bool ok() const noexcept;
```

## What It Does

Returns `failed_step().empty()` -- whether every step in the run
completed. With [`ProcedureOptions::fail_fast`](../ProcedureOptions.md)
at its default `true`, a `ProcedureResult` is only ever reachable with
`ok() == true`, since a failing step's exception propagates out of
[`run`](../Procedure/run.md) instead of producing a result at all; `ok()`
only has something to say when `fail_fast` is `false`.

## Parameters

None.

## Returns

`true` or `false`.

## Errors

None.

## Practical Examples

### Example 1

A normal, successful run.

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
  std::cout << "ok=" << out.ok() << "\n";
  return 0;
}
```

Output

```text
ok=1
```

### Example 2

A failed step under `fail_fast = false` makes `ok()` report `false`.

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
  std::cout << "ok=" << out.ok() << "\n";
  return 0;
}
```

Output

```text
ok=0
```

### Example 3

A sink-only procedure with nothing to fail is `ok()`.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink")),
  });

  const ai::ProcedureResult out = proc.run();
  std::cout << "ok=" << out.ok() << "\n";
  return 0;
}
```

Output

```text
ok=1
```

## See Also

- [`failed_step`](failed_step.md) -- which step, when `ok()` is `false`.
- [`ProcedureOptions::fail_fast`](../ProcedureOptions.md) -- what makes
  this question reachable as `false` at all.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="metrics.html">
    <span class="label">Previous</span>
    <span class="title">metrics</span>
  </a>
  <a class="next-link" href="trace.html">
    <span class="label">Next</span>
    <span class="title">trace</span>
  </a>
</div>
<!-- apiref:nav:end -->
