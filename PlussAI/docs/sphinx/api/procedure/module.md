# `pluss::ai::procedure` (module)

`module` owns the raw-module escape hatch: wrapping any object with a
`.forward(const Series&) -> Series` method as a `Map`+`Infer`-capability
[`Step`](core/Step.md) with no persistence of its own -- the
least-guided of the four adapter families, kept for a transform that
genuinely does not need to be a full model.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`step`](module/step.md) | wraps any `Series -> Series` object as an unpersisted `Step` |

## How To Call It

Wrap a `pluss::nn` module as a step. The program prints the run status and the sum of the ReLU output, which is 2 + 4 = 6.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/procedure.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{-1.0, 2.0, -3.0, 4.0}, {4}, cfg);

  pluss::nn::ReLU relu{};
  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("relu", "x", "positive", relu),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "ok = " << (out.ok() ? "true" : "false") << "\n";
  std::cout << "positive sum = " << out.get<pluss::Series>("positive").sum().item<double>() << "\n";
  return 0;
}
```

Output

```text
ok = true
positive sum = 6
```

Any object with a matching `.forward` method works, not only real `nn`
layers -- a plain struct with nothing else on it satisfies the concept
just as well. Reach for [`ice`](ice.md) instead when the thing being
wrapped is, or could be, a full `ice::Model`; `module` never persists.

## Compile-Time Contract

The module must provide a callable `forward(const Series&)` whose result is
convertible to `Series`. This is checked by the `SeriesForwardModule` concept,
so a wrong signature fails at compilation rather than creating a step that can
only fail later. Runtime validation still checks that the named input artifact
actually holds a Series.

The module is moved into the runner closure and may retain parameters or other
state. `RunOptions::mode` is ignored because the contract exposes no separate
learning or assessment entry point. A stateful `forward` therefore remains the
module author's responsibility; procedure does not infer mutation semantics.

## When Not To Use It

Do not wrap a trainable model here merely because it has `forward()`. Doing so
discards its learning, assessment, readiness, checkpoint, and restoration
contracts. The Ice adapter preserves those behaviors. Likewise, use a plain
callable for ordinary arithmetic that does not benefit from a named module
type.

## Execution And Platform

The adapter forwards the original Series handle. It does not flatten, cast,
copy to CPU, or change platform. Vectorization and accelerator behavior are
therefore exactly those implemented by the module's own `forward`. Procedure's
overhead is limited to artifact lookup, one variant check, runner dispatch,
and optional audit metadata.

## Related Pages

- [`ice`](ice.md) -- the guided path for full models.
- [`registry`](registry.md) -- adding persistence a `module` step
  cannot provide on its own.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

module/Introduction
module/step
```
<!-- apiref:toctree:end -->
