<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4a9d4c071368 -->
# `pluss::ai::Step::reloadable`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
bool reloadable() const noexcept;
```

## What It Does

Returns `persist().reloadable` -- whether [`procedure::load`](../../persist/load.md)
is expected to be able to reconstruct this step's component from its
saved artifact. A step can be [`persistable`](persistable.md) (its state
gets written on [`save`](../Procedure/save.md)) without being
`reloadable` -- useful for a component that is meaningful to inspect
after the fact (an audit record) but has no defined "load me back into a
running `Step`" path.

## Parameters

None.

## Returns

`true` or `false`.

## Errors

None.

## Practical Examples

### Example 1

A sink step is reloadable -- loading one back just needs its target
name.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "reloadable=" << s.reloadable() << "\n";
  return 0;
}
```

Output

```text
reloadable=1
```

### Example 2

A plain callable step with no persist spec is not reloadable (and not
persistable either -- see [`persistable`](persistable.md)).

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "reloadable=" << s.reloadable() << "\n";
  return 0;
}
```

Output

```text
reloadable=0
```

### Example 3

A hand-attached [`StepPersistSpec`](../StepPersistSpec.md) with
`reloadable = true` set explicitly.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.key = "my_key";
  persist.reloadable = true;
  const ai::Step s = ai::procedure::step("custom", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "reloadable=" << s.reloadable() << "\n";
  return 0;
}
```

Output

```text
reloadable=1
```

## See Also

- [`persistable`](persistable.md) -- the coarser, graph-level flag.
- [`procedure::load`](../../persist/load.md) -- what actually reads this.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="persistable.html">
    <span class="label">Previous</span>
    <span class="title">persistable</span>
  </a>
  <a class="next-link" href="run.html">
    <span class="label">Next</span>
    <span class="title">run</span>
  </a>
</div>
<!-- apiref:nav:end -->
