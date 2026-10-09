<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6e989ce2c375 -->
# `pluss::ai::Step::component`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
const std::string& component() const noexcept;
```

## What It Does

Returns `options().component` -- a short tag identifying what kind of
thing this step wraps, recorded in every
[`StepTrace`](../StepTrace.md)/[`procedure::load`](../../persist/load.md)
round-trip so a saved bundle's manifest can record (and later verify)
what loader a step needs. Every built-in adapter sets this to a fixed
value for its own kind (for example `"lymph::workflow::Policy"` or
`"lymph::models::Model"` for [`lymph`](../../lymph.md) steps); a plain
[`callable`](../../callable.md) step without a registry key uses the
fixed tag `"callable"`, and a [`SinkSpec`](../SinkSpec.md)-backed step
uses its own target name.

## Parameters

None.

## Returns

A `const std::string&`, possibly empty for a hand-built step that never
set it.

## Errors

None.

## Practical Examples

### Example 1

A plain callable step's fixed component tag.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "component=" << s.component() << "\n";
  return 0;
}
```

Output

```text
component=callable
```

### Example 2

A sink step's component is its own target name.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "component=" << s.component() << "\n";
  return 0;
}
```

Output

```text
component=procedure.doc.sink
```

### Example 3

A hand-built step can set any tag directly via `StepOptions::component`.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepOptions options{};
  options.component = "my_component";
  const ai::Step s = ai::procedure::step("named", options);

  std::cout << "component=" << s.component() << "\n";
  return 0;
}
```

Output

```text
component=my_component
```

## See Also

- [`StepTrace::component`](../StepTrace.md) -- where this tag shows up
  in a run's audit trail.
- [`registry`](../../registry.md) -- custom components get their own
  tag from their bound key.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="capability_mask.html">
    <span class="label">Previous</span>
    <span class="title">capability_mask</span>
  </a>
  <a class="next-link" href="component_schema_version.html">
    <span class="label">Next</span>
    <span class="title">component_schema_version</span>
  </a>
</div>
<!-- apiref:nav:end -->
