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

```cpp
namespace cls = pluss::ai::classic;

const cls::Status saved = cls::io::save("model.bundle", flow, batch);
// ... later, possibly in a different process ...
const cls::StatusOr<cls::Flow> restored = cls::io::load_and_restore("model.bundle");
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
