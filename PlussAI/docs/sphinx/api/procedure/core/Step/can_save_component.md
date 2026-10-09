<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ce3bc46f0f28 -->
# `pluss::ai::Step::can_save_component`

[Back to `pluss::ai::Step`](../Step.md)

## Signature

```cpp
bool can_save_component() const noexcept;
```

## What It Does

Reports whether `persist().save` holds a callable `Saver` closure. This
is distinct from [`persistable`](persistable.md): a step can be marked
persistable at the `StepOptions` level while having no actual component
artifact to write -- a [`SinkSpec`](../SinkSpec.md)-backed sink step is
exactly this case, since a sink's "component" is just its target name,
already captured in [`persist_fields`](persist_fields.md), with nothing
further to serialize.

## Parameters

None.

## Returns

`true` or `false`.

## Errors

None.

## Practical Examples

### Example 1

A sink step is persistable but has no `Saver` -- there is nothing beyond
its target name to write.

```cpp
#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink"));
  std::cout << "can_save_component=" << s.can_save_component() << "\n";
  return 0;
}
```

Output

```text
can_save_component=0
```

### Example 2

A hand-attached [`StepPersistSpec`](../StepPersistSpec.md) with a real
`Saver` closure set.

```cpp
#include <pluss/ai/procedure.h>

#include <fstream>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepPersistSpec persist{};
  persist.key = "my_key";
  persist.save = [](std::string_view path) {
    std::ofstream out(std::string(path) + ".marker");
    out << "saved";
  };
  const ai::Step s = ai::procedure::step("custom", ai::StepOptions{}, persist,
      [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact { return ai::Artifact("out"); });

  std::cout << "can_save_component=" << s.can_save_component() << "\n";
  return 0;
}
```

Output

```text
can_save_component=1
```

### Example 3

A plain callable step with no persist spec also has no `Saver`.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Step s = ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& x) { return x.mul(10.0); });
  std::cout << "can_save_component=" << s.can_save_component() << "\n";
  return 0;
}
```

Output

```text
can_save_component=0
```

## See Also

- [`save_component`](save_component.md) -- actually invoking the
  `Saver`.
- [`persistable`](persistable.md) -- the coarser, graph-level flag.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="can_change_row_count.html">
    <span class="label">Previous</span>
    <span class="title">can_change_row_count</span>
  </a>
  <a class="next-link" href="capability_mask.html">
    <span class="label">Next</span>
    <span class="title">capability_mask</span>
  </a>
</div>
<!-- apiref:nav:end -->
