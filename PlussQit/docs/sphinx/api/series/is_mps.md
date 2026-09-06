<!-- pluss-hand-authored -->

# Series::is_mps

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_mps() const noexcept
```

## What It Does

Returns `true` when this `Series` currently lives on the **MPS** platform
(Apple's Metal Performance Shaders backend), `false` otherwise (CPU, or a
different accelerator such as CUDA).

It is a metadata read — `platform() == Platform::MPS` — that never transfers
data, never probes the machine, and cannot throw.

`is_mps()` is the **strict, per-backend** test, the MPS counterpart of
[`is_cuda`](is_cuda.md). It sits below [`is_gpu`](is_gpu.md): every MPS
`Series` is `is_gpu()`, but an accelerator `Series` on CUDA is `is_gpu()`
and **not** `is_mps()`. Use `is_mps()` when the next step is MPS-specific —
an MPS-only code path, or a fallback decision for an operation the MPS
backend does not yet implement natively. When the branch is just
"accelerator vs. CPU", use [`is_gpu`](is_gpu.md).

Whether the *machine* supports MPS at all is
`pluss::platform_available(pluss::pMPS)` — check that before calling
`to(pluss::pMPS)`.

## Parameters

None.

## Returns

`bool`
    `true` iff `platform() == Platform::MPS`.

## Gradient

Not applicable — `is_mps()` returns a plain `bool`.

## Examples

Example 1. A fresh `Series` is on the CPU, so `is_mps()` is `false`.

```cpp
auto x = pluss::series::zeros({2});
std::cout << std::boolalpha << "is_mps=" << x.is_mps()
          << " is_gpu=" << x.is_gpu() << "\n";
```

### Output

```text
is_mps=false is_gpu=false
```

Example 2. After `to(pluss::pMPS)` the predicate flips — and so does
[`is_gpu`](is_gpu.md), while [`is_cuda`](is_cuda.md) stays `false`. (Output
from an Apple-Silicon host where MPS is available.)

```cpp
auto x = pluss::series::zeros({2});
auto m = x.to(pluss::pMPS);
std::cout << std::boolalpha
          << "platform()=" << m.platform()
          << " is_cpu=" << m.is_cpu()
          << " is_gpu=" << m.is_gpu()
          << " is_cuda=" << m.is_cuda()
          << " is_mps=" << m.is_mps() << "\n";
```

### Output

```text
platform()=mps is_cpu=false is_gpu=true is_cuda=false is_mps=true
```

Example 3. Moving back to the CPU flips it all the way back.

```cpp
auto x = pluss::series::zeros({2});
auto m = x.to(pluss::pMPS);
auto back = m.to(pluss::pCPU);
std::cout << std::boolalpha
          << "back: platform()=" << back.platform()
          << " is_mps=" << back.is_mps()
          << " is_cpu=" << back.is_cpu() << "\n";
```

### Output

```text
back: platform()=cpu is_mps=false is_cpu=true
```

`x`, `m`, and `back` are three independent `Series`; `to()` produces a new
tensor and never mutates its receiver.

Example 4. Gate a transfer on `pluss::platform_available(pluss::pMPS)` so the
same source runs on any host.

```cpp
auto x = pluss::series::zeros({2});
if (pluss::platform_available(pluss::pMPS)) {
  auto m = x.to(pluss::pMPS);
  std::cout << "moved to MPS; is_mps=" << std::boolalpha << m.is_mps() << "\n";
} else {
  std::cout << "MPS unavailable; staying on CPU (is_mps=" << std::boolalpha
            << x.is_mps() << ")\n";
}
```

### Output

```text
moved to MPS; is_mps=true
```

`is_mps()` reports where a tensor *is*; `platform_available(pMPS)` reports
whether the machine *could* put one there.

## Gotchas And Failure Modes

- `is_mps()` is strict: an accelerator `Series` on CUDA is **not**
  `is_mps()`. Use [`is_gpu`](is_gpu.md) for "on any accelerator".
- It reports *this tensor's* placement, not machine capability — use
  `pluss::platform_available(pluss::pMPS)` before a transfer.
- A binary op requires matching platforms; an MPS tensor cannot be combined
  with a CPU tensor directly (move one with [`to()`](to.md)).
- Not every operation has a native MPS kernel; some fall back internally.
  `is_mps()` tells you the tensor is on MPS, not that every subsequent call
  stays there.
- The predicate never transfers data and never throws.

## Summary

`is_mps()` reports whether a `Series` currently resides on the MPS platform:

- strict per-backend: `platform() == Platform::MPS`
- a subset of [`is_gpu`](is_gpu.md) — every MPS tensor is `is_gpu()`, not
  vice versa
- distinct from `pluss::platform_available(pMPS)` (a machine capability)
- `noexcept`, metadata-only

## See Also

- [`Series::is_gpu`](is_gpu.md): "on any accelerator" — the backend-agnostic
  branch.
- [`Series::is_cuda`](is_cuda.md): the other strict per-backend test.
- [`Series::is_cpu`](is_cpu.md): the exact complement of `is_gpu()`.
- `Series::platform()`: the underlying `Platform` value (`cpu` / `cuda` /
  `mps`).
- [`Series::to`](to.md): move a `Series` onto (or off) MPS.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_leaf.html">
    <span class="label">Previous</span>
    <span class="title">is_leaf</span>
  </a>
  <a class="next-link" href="is_nonzero.html">
    <span class="label">Next</span>
    <span class="title">is_nonzero</span>
  </a>
</div>
