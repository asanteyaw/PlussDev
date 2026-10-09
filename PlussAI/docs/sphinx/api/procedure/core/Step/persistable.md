<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9ead7ba4673e -->
# `pluss::ai::Step::persistable`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
bool persistable() const noexcept;
```

## What It Does

Returns `options().persistable` -- whether this step declares that it
*should* be saved when the enclosing [`Procedure::save`](../Procedure/save.md)
runs. This is a declaration the step's own options carry, independent of
whether a [`StepPersistSpec`](../StepPersistSpec.md) was actually
attached; [`Procedure::save`](../Procedure/save.md) requires every step in
the graph to report `persistable() == true` before it will write anything.

## Parameters

None.

## Returns

`true` or `false`.

## Errors

None.

## Practical Examples

### Example 1

A plain callable step is not persistable by default.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "persistable=" << s.persistable() << "\n";
  return 0;
}
```

Output

```text
persistable=0
```

### Example 2

A sink step is persistable by construction -- see
[`step(name, SinkSpec)`](../../build/step.md).

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "persistable=" << s.persistable() << "\n";
  return 0;
}
```

Output

```text
persistable=1
```

### Example 3

Every built-in engine adapter -- here [`lymph`](../../lymph.md)'s
`Policy` wrapper -- marks its step persistable automatically too.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;

  const ai::Step s = ai::procedure::step("policy", "raw", "clean", lymph::workflow::Policy{.drop_columns = {"x"}});
  std::cout << "persistable=" << s.persistable() << "\n";
  return 0;
}
```

Output

```text
persistable=1
```

## See Also

- [`Procedure::save`](../Procedure/save.md) -- what requires this to be
  `true` across the whole graph.
- [`reloadable`](reloadable.md) / [`frozen`](frozen.md) -- finer-grained
  persistence flags that only matter once `persistable()` is `true`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="persist_schema_version.html">
    <span class="label">Previous</span>
    <span class="title">persist_schema_version</span>
  </a>
  <a class="next-link" href="reloadable.html">
    <span class="label">Next</span>
    <span class="title">reloadable</span>
  </a>
</div>
<!-- apiref:nav:end -->
