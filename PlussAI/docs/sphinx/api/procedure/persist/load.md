# `pluss::ai::procedure::load`

Reloads a `Procedure` from a directory bundle written by
`Procedure::save` -- re-verifying every component's checksum and
routing each step through its built-in or registry-bound
`StepLoader`/`IceFactory`.

## Signature

```cpp
pluss::ai::Procedure pluss::ai::procedure::load(const std::string& path);
```

## What It Does

Reads `path`'s `manifest.plussproc`, reconstructing each step by
looking its recorded [`persist_key`](../core/Step/persist_key.md) up --
first among the built-in loaders every [`lymph`](../lymph.md) adapter
registers automatically, then in the
[`registry`](../registry.md)'s bound custom contracts -- and failing if
neither has one. Each step with a saved component artifact gets that
artifact's checksum recomputed and compared against the manifest's own
recorded value before the loader is even called. After every step is
rebuilt, the whole reconstructed [`Procedure`](../core/Procedure.md) is
validated exactly like a freshly built one would be (see
[`check`](../core/Procedure/check.md)).

## Parameters

`const std::string& path`
: The bundle directory a prior [`save`](../core/Procedure/save.md) call
  wrote.

## Returns

A reconstructed [`Procedure`](../core/Procedure.md) value.

## Errors

- Throws if `manifest.plussproc` cannot be opened, or its header/schema
  is unsupported.
- Throws if any step's recorded `persist_key` has no matching built-in
  loader or bound [`registry`](../registry.md) contract.
- Throws if a saved component's recomputed checksum does not match the
  manifest's recorded value.
- Throws if the reconstructed step's contract (component, persistable,
  key, schema versions, reloadable/frozen/custom) does not match what
  the manifest itself recorded.
- Throws if the reconstructed graph fails
  [`check`](../core/Procedure/check.md), or if the manifest's own
  recorded step/link counts do not match what was actually read.

## Practical Examples

### Example 1

A full save/load/run round-trip for a registry-bound callable step.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <filesystem>
#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::bind("procedure.doc.square", [](const pluss::Series& v) { return v.mul(v); });

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{2.0, 3.0, 4.0}, {3}, cfg);
  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("square", "x", "y", [](const pluss::Series& v) { return v.mul(v); }, "procedure.doc.square"),
  });

  const std::string path = "/tmp/plussai_doc_persist_load_example1";
  std::filesystem::remove_all(path);
  proc.save(path);
  const ai::Procedure restored = ai::procedure::load(path);
  const ai::ProcedureResult out = restored.run({ai::procedure::input("x", x)});
  std::cout << "elements=" << out.get<pluss::Series>("y").numel() << "\n";
  std::filesystem::remove_all(path);
  return 0;
}
```

Output

```text
elements=3
```

### Example 2

A manifest hand-edited to reference a key nothing in this process ever
bound throws -- the manifest format is plain, inspectable text, and
`load` treats it as data to verify, not as ground truth to trust.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::bind("procedure.doc.orphaned_key", [](const pluss::Series& v) { return v.mul(2.0); });
  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("double", "x", "y", [](const pluss::Series& v) { return v.mul(2.0); }, "procedure.doc.orphaned_key"),
  });

  const std::string path = "/tmp/plussai_doc_persist_load_example2";
  std::filesystem::remove_all(path);
  proc.save(path);

  std::ifstream in(path + "/manifest.plussproc");
  std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  in.close();
  const std::string from = "persist_key=procedure.doc.orphaned_key";
  const std::string to = "persist_key=procedure.doc.never_bound_at_all";
  contents.replace(contents.find(from), from.size(), to);
  std::ofstream(path + "/manifest.plussproc", std::ios::trunc) << contents;

  bool threw = false;
  try {
    const ai::Procedure broken = ai::procedure::load(path);
    (void)broken;
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "threw=" << threw << "\n";
  std::filesystem::remove_all(path);
  return 0;
}
```

Output

```text
threw=1
```

### Example 3

The reconstructed graph's shape -- step count, in this case -- matches
the original exactly.

```cpp
#include <pluss/ai/procedure.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("store", ai::procedure::sink("procedure.doc.sink")),
  });

  const std::string path = "/tmp/plussai_doc_persist_load_example3";
  std::filesystem::remove_all(path);
  proc.save(path);
  const ai::Procedure restored = ai::procedure::load(path);
  std::cout << "step_count=" << restored.step_count() << "\n";
  std::filesystem::remove_all(path);
  return 0;
}
```

Output

```text
step_count=1
```

## See Also

- [`Procedure::save`](../core/Procedure/save.md) -- the inverse call.
- [`registry`](../registry.md) -- `bind`/`bind_ice`, what a custom
  component's `persist_key` is resolved against.
