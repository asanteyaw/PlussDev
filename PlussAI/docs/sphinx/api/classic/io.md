# `pluss::ai::classic::io`

`io` owns turning a [`Flow`](compose/Flow.md) into a file and back —
[`save`](io/save.md) writes one to a self-contained binary bundle;
[`load`](io/load.md) reads that bundle back as data
([`FlowBundle`](io/FlowBundle.md)), and its siblings `restore` /
`load_and_restore` turn that data back into a runnable `Flow`. Nothing here
trains or predicts; a bundle is a snapshot, and a restored `Flow` behaves
exactly like the one that produced it.

## The Surface

| Function / type | What it does | Reach for it when |
| --- | --- | --- |
| [`save`](io/save.md) | write a `Flow`'s spec, fitted state (or a training-batch fallback), and category maps to one file | persisting a `Flow` you built and fit, to use again later or elsewhere |
| [`load`](io/load.md) | read a bundle file back as a [`FlowBundle`](io/FlowBundle.md) — plain data, no `Flow` built yet | inspecting a saved bundle's metadata/spec without paying to rebuild it |
| `restore` (bundled with [`load`](io/load.md)) | turn an already-loaded `FlowBundle` into a runnable `Flow` | you already called `load` and now want the `Flow` itself |
| `load_and_restore` (bundled with [`load`](io/load.md)) | `load` + `restore` in one call | the common case — you just want the file back as a usable `Flow` |
| [`FlowBundle`](io/FlowBundle.md) | the on-disk contents, read back as plain data | reading `metadata`/`model`/`transforms` directly, or handing a modified bundle to `restore` yourself |

## How To Call It

Save a fitted flow and its training batch to a bundle, then load and restore it. The program removes the bundle afterward and prints whether each step succeeded.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

namespace cls = pluss::ai::classic;

cls::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  return batch;
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const cls::LabeledBatch batch = fixture_batch(cfg);
  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  cls::Flow flow = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts}).move_value();
  flow.learn(batch);
  const auto bundle = (std::filesystem::temp_directory_path() / "classic_io_example.bundle").string();
  const cls::Status saved = cls::io::save(bundle, flow, batch);
  const cls::StatusOr<cls::Flow> restored = cls::io::load_and_restore(bundle);
  std::filesystem::remove(bundle);
  std::cout << "saved = " << (saved.ok() ? "true" : "false") << "\n";
  std::cout << "restored = " << (restored.ok() ? "true" : "false") << "\n";
  return 0;
}
```

Output

```text
saved = true
restored = true
```

## Related Pages

- [`compose`](compose.md) — `Flow`, the type every function here reads
  from or rebuilds.
- [`estimators`](estimators.md) — `EstimatorBase::dump_state` /
  `load_state`, what a `Flow`'s own fitted-state snapshot is built from.
- [`families`](families.md) — `ModelSpec`, part of every bundle.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

io/Introduction
io/FlowBundle
io/save
io/load
```
<!-- apiref:toctree:end -->
