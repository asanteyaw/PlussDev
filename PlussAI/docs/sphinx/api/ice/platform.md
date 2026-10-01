# `pluss::ai::ice::platform`

`platform` owns device-selection policy for `ice` training: the
`TrainPlatform` request enum (`CPU`/`GPU`/`Auto`) and
[`resolve_platform`](platform/resolve_platform.md), which turns a request
into a concrete `pluss::Platform` by querying this machine's real device
availability -- never a compile-time guess.

## The Surface

| Entity | What it gives you |
| --- | --- |
| `TrainPlatform` | the request enum -- `CPU` (default), `GPU` (explicit, warns if absent), `Auto` (silent) |
| [`resolve_platform`](platform/resolve_platform.md) | resolves a `TrainPlatform` against real CUDA/MPS availability |

## How To Call It

```cpp
namespace ice = pluss::ai::ice;

const pluss::Platform cpu = ice::resolve_platform(ice::TrainPlatform::CPU);
const pluss::Platform gpu = ice::resolve_platform(ice::TrainPlatform::GPU);

std::cout << "CPU -> " << cpu << "\n";
std::cout << "GPU -> " << gpu << "\n";
```

```text
CPU -> cpu
GPU -> mps
```

(Run on a machine with MPS available and no CUDA -- `GPU` resolves to
`pCUDA` instead when CUDA is ready, or warns and falls back to `pCPU`
when neither is present.)

Most callers never call `resolve_platform` directly: setting
`TrainOptions{.execution = {.platform = ice::TrainPlatform::GPU}}` and
passing it to [`Model::learn`](model.md) is enough -- `learn` resolves it
internally and moves both the model and the training data there.

## Related Pages

- [`options`](options.md) -- `ExecutionPolicy.platform` is where a
  `TrainPlatform` request is actually configured.
- [`model`](model.md) -- `Model::learn`/`infer`/`assess`/`to` are where a
  resolved platform takes effect.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

platform/Introduction
platform/resolve_platform
```
<!-- apiref:toctree:end -->
