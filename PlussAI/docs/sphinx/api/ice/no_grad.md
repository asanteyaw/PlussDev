# `pluss::ai::ice::no_grad`

`no_grad` owns [`NoGradGuard`](no_grad/NoGradGuard.md), `ice`'s RAII
no-grad scope -- an `ice`-local name kept for source compatibility,
delegating to the same process-wide, thread-local grad-mode switch
[`nn::NoGradGuard`](../nn/grad/NoGradGuard.md) controls.
[`Model::infer`](model/Model/infer.md) and
[`Model::assess`](model/Model/assess.md) are already wrapped in one of
these through [`Coach`](coach.md), so their returned predictions come
back untracked with no extra code at the call site.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`NoGradGuard`](no_grad/NoGradGuard.md) | construct one, and every `Series` operation inside its scope produces an untracked result; destructor restores whatever grad-mode state was active before |

## How To Call It

```cpp
namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

ice::Outline model(nn::Fusion(2, 1));
const pluss::Series x = pluss::make_series({1.0f, 2.0f}, {1, 2});

{
  const ice::NoGradGuard guard;
  std::cout << "inside: " << model.forward(x).source().has_value() << "\n";
}
std::cout << "after:  " << model.forward(x).source().has_value() << "\n";
```

```text
inside: 0
after:  1
```

Constructing a `NoGradGuard` directly is rarely necessary against a
plain [`Model`](model/Model.md)/[`Outline`](outline/Outline.md) --
[`infer`](model/Model/infer.md)/[`assess`](model/Model/assess.md)
already do it internally. Reach for it directly in a hand-rolled loop
built on [`make_optimizer`](optim/make_optimizer.md) that needs its own
inference-only pass, or anywhere else outside `Model`'s own call paths
that should not record a graph.

## Related Pages

- [`nn::NoGradGuard`](../nn/grad/NoGradGuard.md) -- the type this
  delegates to, with the full mechanics.
- [`model`](model.md) -- `Model::infer`/`assess`, already wrapped in
  one of these.
- [`coach`](coach.md) -- where that wrapping actually happens.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

no_grad/Introduction
no_grad/NoGradGuard
```
<!-- apiref:toctree:end -->
