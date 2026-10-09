<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f610391d91c2 -->

# `pluss::ai::ProcedureResult::metrics`

[Back to `pluss::ai::ProcedureResult`](../ProcedureResult.md)

## Signature

```cpp
const std::vector<Artifact>& metrics() const noexcept;
```

## What It Does

Returns a separate list of [`Artifact`](../Artifact.md)s meant for
step-reported metrics, distinct from a step's main
[`artifacts()`](artifacts.md) output. As of this version,
[`Procedure::run`](../Procedure/run.md) never populates it -- every call
returns an empty list, regardless of how the run went. It is documented
here as a real, callable, reserved part of the public surface rather than
omitted, since calling it is valid today and simply always yields
nothing.

## Parameters

None.

## Returns

A `const std::vector<Artifact>&` -- currently always empty.

## Errors

None.

## Practical Examples

### Example 1

A normal multi-step run: `metrics()` is empty regardless.

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
  std::cout << "metrics=" << out.metrics().size() << "\n";
  return 0;
}
```

Output

```text
metrics=0
```

### Example 2

A sink-only run: still empty.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink")),
  });

  const ai::ProcedureResult out = proc.run();
  std::cout << "metrics=" << out.metrics().size() << "\n";
  return 0;
}
```

Output

```text
metrics=0
```

### Example 3

A failed run (under `fail_fast = false`): still empty -- `metrics()`
does not vary with outcome.

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
  std::cout << "metrics=" << out.metrics().size() << "\n";
  return 0;
}
```

Output

```text
metrics=0
```

## See Also

- [`artifacts`](artifacts.md) -- the populated counterpart.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="ok.html">
    <span class="label">Next</span>
    <span class="title">ok</span>
  </a>
</div>
<!-- apiref:nav:end -->
