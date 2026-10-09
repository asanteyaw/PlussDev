<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3534291f3afa -->
# `pluss::ai::Step::inputs`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
const std::vector<std::string>& inputs() const noexcept;
```

## What It Does

Returns `options().inputs` -- the names this step expects to find
already-produced [`Artifact`](../Artifact.md)s under when
[`Procedure::run`](../Procedure/run.md) resolves its upstream
[`Link`](../Link.md)s. An empty list means the step takes no named input
(a sink step with no upstream, or a step whose single implicit input is
resolved positionally rather than by declared port name).

## Parameters

None.

## Returns

A `const std::vector<std::string>&`.

## Errors

None.

## Practical Examples

### Example 1

A single-input callable step declares exactly one name.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "first_input=" << s.inputs().front() << "\n";
  return 0;
}
```

Output

```text
first_input=x
```

### Example 2

A [`Join`](../Introduction.md)-capability step can declare several
named inputs at once.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepOptions join{};
  join.capabilities = {ai::StepCapability::Join};
  join.inputs = {"a", "b", "c"};
  join.outputs = {"sum"};
  const ai::Step s = ai::procedure::step("sum", join,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("sum"); });

  std::cout << "input_count=" << s.inputs().size() << "\n";
  return 0;
}
```

Output

```text
input_count=3
```

### Example 3

A sink step declares no named inputs of its own -- its one upstream
artifact is matched positionally by [`Procedure::run`](../Procedure/run.md)
instead.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "input_count=" << s.inputs().size() << "\n";
  return 0;
}
```

Output

```text
input_count=0
```

## See Also

- [`outputs`](outputs.md) -- the companion port list.
- [`Link`](../Link.md) -- `from_output`/`to_input`, how a link addresses
  one of these names.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="name.html">
    <span class="label">Next</span>
    <span class="title">name</span>
  </a>
</div>
<!-- apiref:nav:end -->
