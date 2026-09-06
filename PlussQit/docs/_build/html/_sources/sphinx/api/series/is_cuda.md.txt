<!-- pluss-hand-authored -->

# Series::is_cuda

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_cuda() const noexcept
```

## What It Does

Returns `true` when this `Series` currently lives on the **CUDA** platform,
`false` otherwise (CPU, or a different accelerator such as MPS).

It is a metadata read — `platform() == Platform::CUDA` — that never
transfers data, never probes the machine, and cannot throw.

`is_cuda()` is the **strict, per-backend** test. It sits below
[`is_gpu`](is_gpu.md): every CUDA `Series` is also `is_gpu()`, but a
`Series` on MPS is `is_gpu()` and **not** `is_cuda()`. Reach for `is_cuda()`
specifically when the work you do next depends on the CUDA backend being the
one in play — a CUDA-only kernel path, a `cudaStream`-aware call, a transfer
policy that only makes sense for CUDA. When the branch is "accelerator vs.
CPU" and the backend identity does not matter, [`is_gpu`](is_gpu.md) is the
right predicate.

As with all the platform predicates, `is_cuda()` reports where *this tensor*
is. Whether the *machine* can run CUDA at all is
`pluss::platform_available(pluss::pCUDA)` — a different question, answered
before you decide to move data there.

## Parameters

None.

## Returns

`bool`
    `true` iff `platform() == Platform::CUDA`.

## Gradient

Not applicable — `is_cuda()` returns a plain `bool`.

## Examples

Example 1. On a host without CUDA, `is_cuda()` is `false` for every
`Series` — including accelerator tensors on a *different* backend. (Output
from an Apple-Silicon host: MPS available, CUDA not.)

```cpp
auto x = pluss::series::zeros({2});
std::cout << std::boolalpha
          << "cpu tensor: is_cuda=" << x.is_cuda() << " is_gpu=" << x.is_gpu() << "\n";
if (pluss::platform_available(pluss::pMPS)) {
  auto m = x.to(pluss::pMPS);
  std::cout << "mps tensor: is_cuda=" << m.is_cuda()
            << " is_gpu=" << m.is_gpu()
            << " is_mps=" << m.is_mps() << "\n";
}
```

### Output

```text
cpu tensor: is_cuda=false is_gpu=false
mps tensor: is_cuda=false is_gpu=true is_mps=true
```

The MPS tensor is on an accelerator (`is_gpu() == true`) but it is emphatically
not CUDA — `is_cuda()` pins the exact backend where `is_gpu()` only says
"off CPU".

Example 2. `is_cuda()` (this tensor) vs
`pluss::platform_available(pluss::pCUDA)` (this machine).

```cpp
auto x = pluss::series::zeros({2});
std::cout << std::boolalpha
          << "x.is_cuda()                        = " << x.is_cuda() << "\n"
          << "platform_available(pCUDA) [machine] = " << pluss::platform_available(pluss::pCUDA) << "\n";
```

### Output

```text
x.is_cuda()                        = false
platform_available(pCUDA) [machine] = false
```

On a CUDA host the second line reads `true` and you may then call
`x.to(pluss::pCUDA)`; afterward `is_cuda()` on the moved tensor reads
`true`. The two lines answer independent questions — "is the backend here"
and "is this tensor on it".

Example 3. `is_cuda()` is `noexcept` and metadata-only — content, dtype, and
shape never affect it, and it is free to call in a hot loop.

```cpp
auto x = pluss::make_series({std::nan(""), 1.0, 2.0});
std::cout << "is_cuda=" << std::boolalpha << x.is_cuda()
          << "  (unaffected by the NaN, the shape, or anything but placement)\n";
```

### Output

```text
is_cuda=false  (unaffected by the NaN, the shape, or anything but placement)
```

## Gotchas And Failure Modes

- `is_cuda()` is strict: an accelerator `Series` on MPS is **not**
  `is_cuda()`. Use [`is_gpu`](is_gpu.md) for "on any accelerator".
- It reports *this tensor's* placement, not the machine's capability. Use
  `pluss::platform_available(pluss::pCUDA)` before deciding to move data to
  CUDA.
- A binary op requires both operands on the same platform; a CUDA tensor and
  a CPU tensor cannot be combined directly (move one with [`to()`](to.md)).
- The predicate never transfers data and never throws.

## Summary

`is_cuda()` reports whether a `Series` currently resides on the CUDA
platform:

- strict per-backend: `platform() == Platform::CUDA`
- a subset of [`is_gpu`](is_gpu.md) — every CUDA tensor is `is_gpu()`, not
  vice versa
- distinct from `pluss::platform_available(pCUDA)` (a machine capability)
- `noexcept`, metadata-only

## See Also

- [`Series::is_gpu`](is_gpu.md): "on any accelerator" — the backend-agnostic
  branch.
- [`Series::is_mps`](is_mps.md): the other strict per-backend test.
- [`Series::is_cpu`](is_cpu.md): the exact complement of `is_gpu()`.
- `Series::platform()`: the underlying `Platform` value (`cpu` / `cuda` /
  `mps`).
- [`Series::to`](to.md): move a `Series` onto (or off) CUDA.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_cpu.html">
    <span class="label">Previous</span>
    <span class="title">is_cpu</span>
  </a>
  <a class="next-link" href="is_floating_point.html">
    <span class="label">Next</span>
    <span class="title">is_floating_point</span>
  </a>
</div>
