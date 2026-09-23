<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 645e59fb829b -->

# `pluss::ai::nn::Module::restore`

[Back to `pluss::ai::nn::Module`](../Module.md)

## Signature

```cpp
Module::RestoreReport nn::Module::restore(const Module::Snapshot& snap, bool strict = true);
```

## Parameters

`const Module::Snapshot& snap`
: A snapshot previously captured by [`snapshot`](snapshot.md) -- on this
  module, on a different instance of the same type, or (in the mismatched
  case) even a structurally different module. Every entry is matched by
  **name**, not by position.

`bool strict`
: When `true` (the default), a snapshot that does not exactly match this
  module's own named parameters and buffers (anything missing, or anything
  in `snap` this module has no matching name for) throws. When `false`, the
  mismatch is reported in the returned `RestoreReport` instead, and every
  name that *does* match is still restored.

## Returns

A `Module::RestoreReport{vector<string> missing, vector<string> unexpected}`.
`missing` lists `"param:<name>"` / `"buffer:<name>"` entries this module has
that `snap` did not provide (left untouched). `unexpected` lists entries
`snap` provided that this module has no matching name for (ignored). Also
sets `is_training()` to `snap.training` unconditionally, even under a
`strict` mismatch that throws -- the throw happens after every matching
entry (and the mode) has already been applied.

## Errors

- `strict == true` and `missing` or `unexpected` is non-empty after matching
  — throws with a message listing every mismatched name.

## Practical Examples

Three examples: a clean round trip, a deliberate mismatch showing the
non-strict report and the strict throw, and restoring a snapshot across a
platform/dtype change.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion layer(2, 2, /*bias=*/false, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), /*seed=*/7);
  std::cout << "original weight: " << layer->weight() << "\n\n";

  pluss::ai::nn::Module::Snapshot snap = layer.snapshot();

  layer->weight().copy_(pluss::Series::zeros({2, 2}, pluss::ScalarType::Float64, pluss::pCPU));
  std::cout << "mutated weight:  " << layer->weight() << "\n\n";

  layer.restore(snap);
  std::cout << "restored weight: " << layer->weight() << "\n";

  return 0;
}
```

Output

```text
original weight: pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[-0.15580425, -0.68336455],
  [0.56676123, 0.11728111]]

mutated weight:  pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, 0],
  [0, 0]]

restored weight: pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[-0.15580425, -0.68336455],
  [0.56676123, 0.11728111]]
```

### Example 2

Restoring `a`'s snapshot onto `b`, a structurally different `Fusion`
(constructed with `bias=false`, so it has no `"bias"` parameter to match
`a`'s). Non-strict mode restores what it can and reports the rest; strict
mode (the default) throws instead.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion a(2, 2);
  nn::Fusion b(2, 2, /*bias=*/false);  // no bias -- fewer registered parameters than `a`

  pluss::ai::nn::Module::Snapshot snap = a.snapshot();

  pluss::ai::nn::Module::RestoreReport report = b.restore(snap, /*strict=*/false);
  std::cout << "non-strict restore -- missing:\n";
  for (auto& name : report.missing) std::cout << "  " << name << "\n";
  std::cout << "non-strict restore -- unexpected:\n";
  for (auto& name : report.unexpected) std::cout << "  " << name << "\n";

  std::cout << "\nstrict restore of a mismatched snapshot throws:\n";
  try {
    b.restore(snap, /*strict=*/true);
    std::cout << "(did not throw)\n";
  } catch (const std::exception& e) {
    std::cout << "  caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
non-strict restore -- missing:
non-strict restore -- unexpected:
  param:bias

strict restore of a mismatched snapshot throws:
  caught: pluss::nn::Module::restore: checkpoint mismatch unexpected=[param:bias]
```

`missing` is empty because `b`'s one parameter (`"weight"`) *is* present in
`snap`. `unexpected` names `"bias"` because `snap` has it but `b` has nowhere
to put it. `nn` reports contract violations by throwing a C++ exception
directly (unlike `classic`, which returns a `Status`/`StatusOr`) --
`restore`'s `strict` parameter is the one place this page's contract lets you
choose between the two styles for the same call.

### Example 3

`restore` converts each snapshot value to the *live* module's current
platform and dtype before assigning (`assign_like`) -- restoring a Float32
snapshot onto a module that has since moved to Float64 leaves it at Float64,
with the snapshot's values.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::ScalarType;

  nn::Fusion layer(2, 2, /*bias=*/false);  // Float32 by default
  pluss::ai::nn::Module::Snapshot snap = layer.snapshot();
  std::cout << "snapshot value is Float32=" << std::boolalpha
            << (snap.parameters[0].value.scalar_type() == ScalarType::Float32) << "\n";

  layer.to(ScalarType::Float64);  // move the live module to Float64 first
  layer.restore(snap);            // restore the Float32 snapshot onto it
  std::cout << "after restoring a Float32 snapshot onto a Float64 module, weight is Float64="
            << (layer->weight().scalar_type() == ScalarType::Float64) << "\n";

  return 0;
}
```

Output

```text
snapshot value is Float32=true
after restoring a Float32 snapshot onto a Float64 module, weight is Float64=true
```

## See Also

- [`snapshot`](snapshot.md) -- produces the value this method consumes.
- [`train`](train.md) -- `restore` also sets `is_training()`, unconditionally.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="parameters.html">
    <span class="label">Previous</span>
    <span class="title">parameters</span>
  </a>
  <a class="next-link" href="snapshot.html">
    <span class="label">Next</span>
    <span class="title">snapshot</span>
  </a>
</div>
<!-- apiref:nav:end -->
