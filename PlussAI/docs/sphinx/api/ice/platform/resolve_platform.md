# `pluss::ai::ice::resolve_platform`

Resolves a [`TrainPlatform`](../platform.md) request (`CPU`/`GPU`/`Auto`)
against a real runtime device query, warning and falling back to CPU for
`GPU` when no accelerator is present.

## Signature

```cpp
pluss::Platform pluss::ai::ice::resolve_platform(pluss::ai::ice::TrainPlatform requested);
```

## What It Does

Converts a `TrainPlatform` request into a concrete
`pluss::Platform` by querying this machine's
actual device availability (`pluss::cuda::is_ready()` /
`pluss::mps::is_ready()`), never a compile-time guess:

- `TrainPlatform::CPU` -- always resolves to `pluss::pCPU`, regardless of
  what accelerators are available. This is `TrainOptions`' own default, so
  training stays on CPU unless a caller opts in deliberately.
- `TrainPlatform::GPU` -- an explicit request for acceleration: `pCUDA` if
  CUDA is ready, else `pMPS` if MPS is ready, else `pluss::validate::warn`
  fires and the result falls back to `pCPU`. Use this when the *absence*
  of a GPU is worth telling the caller about.
- `TrainPlatform::Auto` -- the same fallback order as `GPU` (CUDA, then
  MPS, then CPU), but silent about it. Use this when either device is
  genuinely fine and a warning would just be noise.

[`Model::learn`](../model/Model.md) calls this once per call (from
`TrainOptions::execution.platform`) and moves both the model and the
training/validation data to the result; `infer`/`assess` instead match
wherever the model's parameters currently live, so a model trained with
`TrainPlatform::GPU` stays usable without the caller re-specifying a
device on every later call.

## Parameters

`TrainPlatform requested`
: Which platform policy to resolve -- see `## What It Does` above for
  each value's fallback behavior.

## Returns

`pluss::Platform` -- the concrete platform this request resolved to on
this machine right now (`pCPU`, `pCUDA`, or `pMPS`).

## Errors

None thrown. `TrainPlatform::GPU` with no accelerator present warns via
`pluss::validate::warn` rather than throwing, then returns `pCPU`.

## Practical Examples

Three examples, one per `TrainPlatform` value, run on a machine with MPS
available and no CUDA.

### Example 1

`TrainPlatform::CPU` always resolves to `pCPU`, independent of what
accelerators exist.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Platform cpu = ice::resolve_platform(ice::TrainPlatform::CPU);
  std::cout << "CPU -> " << cpu << "\n";

  return 0;
}
```

Output

```text
CPU -> cpu
```

### Example 2

`TrainPlatform::GPU` on this machine (MPS available, no CUDA) resolves to
`pMPS`, silently -- a machine with neither accelerator would instead see
a warning and `pCPU` back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Platform gpu = ice::resolve_platform(ice::TrainPlatform::GPU);
  std::cout << "GPU -> " << gpu << "\n";

  return 0;
}
```

Output

```text
GPU -> mps
```

### Example 3

`TrainPlatform::Auto` follows the same CUDA-then-MPS-then-CPU order as
`GPU` -- here also landing on `pMPS`, the same result `GPU` produced,
confirming the two requests share a fallback order and differ only in
whether an absent accelerator is worth a warning.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Platform autop = ice::resolve_platform(ice::TrainPlatform::Auto);
  std::cout << "Auto -> " << autop << "\n";

  std::cout << "cuda::is_ready() = " << pluss::cuda::is_ready() << "\n";
  std::cout << "mps::is_ready()  = " << pluss::mps::is_ready() << "\n";

  return 0;
}
```

Output

```text
Auto -> mps
cuda::is_ready() = 0
mps::is_ready()  = 1
```

## See Also

- [`TrainOptions`](../options/TrainOptions.md) -- `execution.platform` is
  the `TrainPlatform` this function resolves.
- [`Model::learn`](../model/Model.md) -- calls this once per call and
  moves the model/data to the result.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
