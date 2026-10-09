<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4372aee61263 -->
# `pluss::ai::Procedure::save`

[Back to `pluss::ai::Procedure`](../Procedure.md)

## Signature

```cpp
void save(std::string_view path) const;
```

## What It Does

Writes the whole procedure -- wiring and every persistable step's
component state -- to `path` as a directory bundle: a text manifest
(`manifest.plussproc`), one component artifact per persistable step under
`steps/<index>_<name>/`, and an `audit/` directory with checksums,
lineage, contracts, and a human-readable summary. See
[`persist`](../../persist.md) for the full bundle format and
[`procedure::load`](../../persist/load.md) for the inverse. `path` is
created if it does not already exist; an existing directory at `path` is
overwritten.

## Parameters

`std::string_view path`
: The destination directory. Created (recursively) if missing.

## Returns

Nothing (`void`).

## Errors

- Throws if any step is not [`persistable`](../Step/persistable.md) --
  every step in the graph must declare persistence (via
  [`StepPersistSpec`](../StepPersistSpec.md), a built-in adapter's own
  default, or [`SinkSpec`](../SinkSpec.md)) before the procedure as a
  whole can be saved.
- Throws if `path` cannot be created or written to (permissions, a
  non-directory file already at that path, and similar filesystem
  failures).

## Practical Examples

### Example 1

A one-sink procedure (sink steps are persistable by construction --
see [`step(name, SinkSpec)`](../../build/step.md)) saves a manifest file.

```cpp
#include <pluss/ai/procedure.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink")),
  });

  const std::string path = "/tmp/plussai_doc_procedure_save_example1";
  std::filesystem::remove_all(path);
  proc.save(path);
  std::cout << "manifest_exists=" << std::filesystem::exists(path + "/manifest.plussproc") << "\n";
  std::filesystem::remove_all(path);
  return 0;
}
```

Output

```text
manifest_exists=1
```

### Example 2

The `audit/` directory's `checksums.txt` is written alongside the
manifest.

```cpp
#include <pluss/ai/procedure.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink")),
  });

  const std::string path = "/tmp/plussai_doc_procedure_save_example2";
  std::filesystem::remove_all(path);
  proc.save(path);
  std::cout << "audit_exists=" << std::filesystem::exists(path + "/audit/checksums.txt") << "\n";
  std::filesystem::remove_all(path);
  return 0;
}
```

Output

```text
audit_exists=1
```

### Example 3

`save` then [`procedure::load`](../../persist/load.md) round-trips the
graph shape -- the reloaded procedure has the same `step_count()`.

```cpp
#include <pluss/ai/procedure.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink")),
  });

  const std::string path = "/tmp/plussai_doc_procedure_save_example3";
  std::filesystem::remove_all(path);
  proc.save(path);
  const ai::Procedure reloaded = ai::procedure::load(path);
  std::cout << "reloaded_step_count=" << reloaded.step_count() << "\n";
  std::filesystem::remove_all(path);
  return 0;
}
```

Output

```text
reloaded_step_count=1
```

## See Also

- [`procedure::load`](../../persist/load.md) -- the inverse call.
- [`persist`](../../persist.md) -- the full bundle layout and its
  checksum/schema guarantees.
- [`StepPersistSpec`](../StepPersistSpec.md) -- what makes a step
  persistable in the first place.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="run.html">
    <span class="label">Previous</span>
    <span class="title">run</span>
  </a>
  <a class="next-link" href="step_count.html">
    <span class="label">Next</span>
    <span class="title">step_count</span>
  </a>
</div>
<!-- apiref:nav:end -->
