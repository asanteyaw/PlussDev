<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6a6560dfb89b -->
# `pluss::ai::Step::outputs`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
const std::vector<std::string>& outputs() const noexcept;
```

## What It Does

Returns `options().outputs` -- the names this step's own
[`run`](run.md) result may be published under. A step with exactly one
declared output names its result explicitly; a step with none falls back
to naming its single output `Artifact` after the step itself (see
[`Step::run`](run.md)'s own `default_output_name_` fallback).

## Parameters

None.

## Returns

A `const std::vector<std::string>&`.

## Errors

None.

## Practical Examples

### Example 1

A single-output callable step declares exactly one name.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "first_output=" << s.outputs().front() << "\n";
  return 0;
}
```

Output

```text
first_output=scaled
```

### Example 2

A step can declare several output names, for runners that return more
than one logical result packed into separate `Artifact`s (via the
multi-artifact [`ProcedureResult`](../ProcedureResult.md) path).

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepOptions options{};
  options.outputs = {"a", "b"};
  const ai::Step s = ai::procedure::step("multi", options);

  std::cout << "output_count=" << s.outputs().size() << "\n";
  return 0;
}
```

Output

```text
output_count=2
```

### Example 3

A sink step declares no output name -- its own name doubles as the
published artifact's name instead (see [`run`](run.md)).

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "output_count=" << s.outputs().size() << "\n";
  return 0;
}
```

Output

```text
output_count=0
```

## See Also

- [`inputs`](inputs.md) -- the companion port list.
- [`run`](run.md) -- where an undeclared output falls back to the step's
  own name.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="options.html">
    <span class="label">Previous</span>
    <span class="title">options</span>
  </a>
  <a class="next-link" href="persist.html">
    <span class="label">Next</span>
    <span class="title">persist</span>
  </a>
</div>
<!-- apiref:nav:end -->
