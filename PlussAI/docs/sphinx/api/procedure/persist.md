# `pluss::ai::procedure` (persist)

`persist` owns the directory-bundle format
[`Procedure::save`](core/Procedure/save.md) writes and
[`procedure::load`](persist/load.md) reads: a text manifest, one
component artifact per persistable step, and a human-readable audit
directory.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`load`](persist/load.md) | reconstructs a `Procedure` from a bundle `save` wrote |

## How To Call It

Save a procedure whose step is bound to a registry key, reload it, and run the restored graph. The bundle is removed once it has been loaded.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/procedure.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  auto scaler = [](const pluss::Series& v) { return v.mul(10.0); };
  ai::procedure::bind("my_scaler_v1", scaler);
  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", scaler, std::string("my_scaler_v1")),
  });

  const auto bundle = std::filesystem::temp_directory_path() / "procedure_persist_example";
  proc.save(bundle.string());
  const ai::Procedure restored = ai::procedure::load(bundle.string());
  std::filesystem::remove_all(bundle);

  const ai::ProcedureResult out = restored.run({ai::procedure::input("x", x)});
  std::cout << "restored ok = " << (out.ok() ? "true" : "false") << "\n";
  std::cout << "scaled sum = " << out.get<pluss::Series>("scaled").sum().item<double>() << "\n";
  return 0;
}
```

Output

```text
restored ok = true
scaled sum = 60
```

Every step's saved component artifact gets its checksum recomputed and
compared on `load`, and every step's reconstructed contract is checked
against what the manifest itself recorded -- a bundle that was hand-edited
or partially corrupted fails loudly rather than loading silently wrong.

## Bundle Contents

| Path | Purpose |
| --- | --- |
| `manifest.plussproc` | Procedure options, step/link topology, ports, capabilities, persistence schemas, metadata fields, and checksums |
| `steps/` | Component artifacts and adapter sidecars |
| `audit/summary.txt` | Human-readable counts and contract summary for inspection without execution |

Transient run inputs, outputs, warnings, traces, and audit timings are not
saved. Persistence captures the reusable graph and component state, not a
historical `ProcedureResult`.

## Save And Replace Semantics

`save()` expects a directory bundle. It creates missing directories and writes
the current manifest and component artifacts. A path that already names a
regular file is rejected. Callers responsible for crash-safe deployment should
save to a staging directory, verify it by loading, and atomically publish that
directory according to their filesystem policy; procedure does not claim an
atomic replacement transaction across every bundle file.

Every step must declare itself persistable before the graph can be saved. A
persistable step may still have no component file when its entire state is
metadata, as with a sink. Conversely, a raw callable or module without a
reload contract prevents saving the graph rather than being silently omitted.

## Restoration Boundary

`load()` validates the manifest header and schema, file checksums, step/link
counts, loader availability, custom compatibility gates, and the reconstructed
step contract. Built-in adapters provide built-in loaders; custom callables
and components require their registry bindings to exist in the loading process
first. Restored graphs are passed through ordinary `procedure::make`, so cycle
and endpoint validation is not bypassed.

Treat a bundle as executable configuration: only load bundles from trusted
deployment channels, even though checksums detect corruption. A checksum
establishes integrity relative to the manifest, not the authority of whoever
created both files.

## Related Pages

- [`core`](core.md) -- `Procedure::save`, the other half of this
  story.
- [`registry`](registry.md) -- where a custom component's key is
  resolved during `load`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

persist/Introduction
persist/load
```
<!-- apiref:toctree:end -->
