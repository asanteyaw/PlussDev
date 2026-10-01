<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6b71a9795c6c -->

# `pluss::ai::nn::Module::to`

[Back to `pluss::ai::nn::Module`](../Module.md)

## Signature

```cpp
Module& nn::Module::to(Platform platform);
Module& nn::Module::to(ScalarType dtype);
Module& nn::Module::to(Platform platform, ScalarType dtype);
```

## What It Does

Moves and/or converts every parameter and buffer this module owns --
recursively, through every registered child -- to a new platform, a new
dtype, or both in one call. This is how a whole network built from
[`Schedule`](../Schedule.md) moves to MPS or drops to a lower-precision
dtype: call `to` once on the root and the walk reaches every leaf layer's
`weight`/`bias`/other registered state. Layers whose behavior depends on
`Config` (for example [`PReLU`](../../activation/PReLU.md)'s `options().cfg`
or [`LayerNorm`](../../normalization/LayerNormOptions.md)'s) keep that
options state in sync automatically via their own
`on_platform_changed_`/`on_dtype_changed_` overrides, called after the
parameter/buffer walk completes.

## Parameters

`Platform platform`
: The device to move every registered parameter and buffer to (`pCPU`,
  `pMPS`, ...), recursively through every registered child.

`ScalarType dtype`
: The scalar type to convert every registered parameter and buffer to,
  recursively. Gradient tracking (`tracks_slope()`) is preserved across the
  conversion for parameters -- `to` never silently untracks a parameter the
  way a raw reassignment would (see the category
  [Introduction](../Introduction.md)'s registration-discipline note).

## Returns

`Module&` — a reference to `*this`, for chaining. Every overload mutates in
place; there is no separate value-returning form.

## Errors

None from `Module::to` itself. A concrete layer's own
`on_platform_changed_`/`on_dtype_changed_` override (called after the
parameter/buffer/child walk) may fail for a platform/dtype combination it
does not support — see that layer's own page.

## Practical Examples

Three examples: dtype then combined platform+dtype conversion on a single
layer, recursive conversion across a composed network's every child, and
proof that gradient tracking survives the conversion.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::ScalarType;

  nn::Fusion layer(3, 2);
  std::cout << "before: " << layer->weight() << "\n\n";

  layer.to(ScalarType::Float64);
  std::cout << "after to(Float64): " << layer->weight() << "\n\n";

  layer.to(pluss::pMPS, ScalarType::Float32);
  std::cout << "after to(pMPS, Float32): " << layer->weight() << "\n";

  return 0;
}
```

Output

```text
before: pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[0.041656256, -0.53124946, -0.3967123],
  [0.55133665, -0.45432308, 0.065687835]]

after to(Float64): pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.041656256, -0.53124946, -0.3967123],
  [0.55133665, -0.45432308, 0.065687835]]

after to(pMPS, Float32): pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=mps, numel=6, contiguous=true)
data=[[0.041656256, -0.53124946, -0.3967123],
  [0.55133665, -0.45432308, 0.065687835]]
```

Values are unchanged across every conversion (up to floating-point
round-trip precision) — `to` changes representation, never content.

### Example 2

`to` recurses into every registered child, not just the module it is called
on directly -- both stages of a two-layer `Schedule` convert in one call.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::ScalarType;

  nn::Schedule net;
  net.append("a", nn::Fusion(3, 4));
  net.append("b", nn::Fusion(4, 2));

  net.to(ScalarType::Float64);
  for (auto& named : net.named_parameters()) {
    std::cout << "'" << named.name << "' is Float64=" << std::boolalpha
               << (named.series->scalar_type() == ScalarType::Float64) << "\n";
  }

  return 0;
}
```

Output

```text
'a.weight' is Float64=true
'a.bias' is Float64=true
'b.weight' is Float64=true
'b.bias' is Float64=true
```

### Example 3

Gradient tracking (`tracks_slope()`) is preserved across a `to` conversion --
`to` never reassigns the parameter's underlying `Series` wholesale the way a
naive `weight_ = weight_.to(dtype)` would, which would silently drop tracking
(see the category [Introduction](../Introduction.md)'s registration-
discipline note).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::ScalarType;

  nn::Fusion layer(3, 2);
  std::cout << "before to(): weight.tracks_slope()=" << std::boolalpha << layer->weight().tracks_slope() << "\n";

  layer.to(ScalarType::Float64);
  std::cout << "after to(Float64): weight.tracks_slope()=" << layer->weight().tracks_slope()
            << " -- gradient tracking survives the conversion\n";

  return 0;
}
```

Output

```text
before to(): weight.tracks_slope()=true
after to(Float64): weight.tracks_slope()=true -- gradient tracking survives the conversion
```

## See Also

- [`snapshot`](snapshot.md) / [`restore`](restore.md) — snapshots capture
  values independent of platform/dtype; restoring onto a module at a
  different platform/dtype converts automatically (see `restore`'s
  `assign_like` behavior).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="snapshot.html">
    <span class="label">Previous</span>
    <span class="title">snapshot</span>
  </a>
  <a class="next-link" href="train.html">
    <span class="label">Next</span>
    <span class="title">train</span>
  </a>
</div>
<!-- apiref:nav:end -->
