<!-- pluss-hand-authored -->

# Series::is_gpu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_gpu() const noexcept
```

## What It Does

Returns `true` when this `Series` is **not on the CPU** — that is, when it
lives on any accelerator backend.

The definition is literally the negation of [`is_cpu`](is_cpu.md):

```cpp
bool is_gpu() const noexcept { return !is_cpu(); }
```

so `is_cpu()` and `is_gpu()` partition every `Series` — exactly one is
`true`. For the current backend set (CPU, CUDA, MPS) that makes `is_gpu()`
equivalent to `is_cuda() || is_mps()`, but the *definition* is "off CPU", not
"one of these two named accelerators" — any future non-CPU backend would also
report `is_gpu() == true`.

`is_gpu()` is the **backend-agnostic** placement test. Use it when the branch
is "does this tensor need a host round-trip / can it stay on device", and the
specific accelerator does not change the code. When it does — a CUDA-only or
MPS-only path — use [`is_cuda`](is_cuda.md) or [`is_mps`](is_mps.md) to pin
which one.

Like its siblings, `is_gpu()` reports where *this tensor* currently is, never
whether the machine *has* an accelerator (that is
`pluss::platform_available(pluss::pCUDA)` / `pluss::platform_available(pluss::pMPS)`),
and it never moves data — [`to(Platform)`](to.md) does that.

## Parameters

None.

## Returns

`bool`
    `true` iff `platform() != Platform::CPU`.

## Gradient

Not applicable — `is_gpu()` returns a plain `bool`.

## Examples

Example 1. `is_gpu()` is exactly `!is_cpu()` — the two always disagree.

```cpp
auto x = pluss::series::zeros({2});
std::cout << std::boolalpha
          << "is_cpu=" << x.is_cpu()
          << "  !is_cpu=" << (!x.is_cpu())
          << "  is_gpu=" << x.is_gpu() << "\n";
```

### Output

```text
is_cpu=true  !is_cpu=false  is_gpu=false
```

Example 2. Move a tensor to an accelerator and `is_gpu()` flips to `true`,
alongside the specific backend predicate. (Output from an Apple-Silicon host
where MPS is available.)

```cpp
auto x = pluss::series::zeros({2});
auto m = x.to(pluss::pMPS);
std::cout << std::boolalpha
          << "is_gpu=" << m.is_gpu()
          << " is_cuda=" << m.is_cuda()
          << " is_mps=" << m.is_mps() << "\n";
```

### Output

```text
is_gpu=true is_cuda=false is_mps=true
```

`is_gpu()` tells you the tensor is off the CPU; [`is_mps`](is_mps.md) tells
you *which* accelerator it landed on. On a CUDA host the same move to
`pluss::pCUDA` would give `is_gpu=true is_cuda=true is_mps=false`.

Example 3. `is_gpu()` is the branch to write when the backend identity does
not matter — for example "materialize on the host only if it isn't already
there".

```cpp
auto stay_or_fetch = [](const pluss::Series& s) {
  return s.is_gpu() ? s.to(pluss::pCPU) : s;   // no-op when already on CPU
};
auto x = pluss::series::zeros({2});
auto y = stay_or_fetch(x);
std::cout << std::boolalpha << "y.is_cpu()=" << y.is_cpu() << "\n";
```

### Output

```text
y.is_cpu()=true
```

Example 4. An **undefined** `Series` reports `is_gpu() == false` — because
`platform()` defaults to `Platform::CPU`, which makes `is_cpu()` `true`.

```cpp
pluss::Series undef;
std::cout << std::boolalpha
          << "defined()=" << undef.defined()
          << " is_cpu=" << undef.is_cpu()
          << " is_gpu=" << undef.is_gpu() << "\n";
```

### Output

```text
defined()=false is_cpu=true is_gpu=false
```

So `is_gpu() == false` does not by itself mean "safe to use on the host" —
check `defined()` if the tensor might be uninitialized.

Example 5. `is_gpu()` does not report machine capability — a CPU-only host
still has every `Series` answer `false` here.

```cpp
auto x = pluss::series::zeros({2});
std::cout << std::boolalpha
          << "x.is_gpu()                          = " << x.is_gpu() << "\n"
          << "platform_available(pCUDA) [machine] = " << pluss::platform_available(pluss::pCUDA) << "\n"
          << "platform_available(pMPS)  [machine] = " << pluss::platform_available(pluss::pMPS) << "\n";
```

### Output

```text
x.is_gpu()                          = false
platform_available(pCUDA) [machine] = false
platform_available(pMPS)  [machine] = true
```

`x.is_gpu()` is `false` even though this machine *can* run MPS — because
`x` itself was never moved there.

## Gotchas And Failure Modes

- `is_gpu()` is defined as `!is_cpu()`, not as `is_cuda() || is_mps()`. They
  coincide for the current backends, but the meaning is "off CPU".
- It does not tell you *which* accelerator — use [`is_cuda`](is_cuda.md) /
  [`is_mps`](is_mps.md) when the backend matters.
- It does not tell you whether the machine has an accelerator — use
  `pluss::platform_available(p)` for that.
- An undefined `Series` reports `is_gpu() == false`. Pair with `defined()`
  when that matters.
- A binary op still requires both operands on the *same* platform;
  `is_gpu()` being `true` for both does not mean they are on the *same*
  accelerator.

## Summary

`is_gpu()` reports whether a `Series` is on any accelerator:

- defined as the exact negation of [`is_cpu`](is_cpu.md) — "not on CPU"
- the backend-agnostic placement branch; use [`is_cuda`](is_cuda.md) /
  [`is_mps`](is_mps.md) when the specific backend matters
- per-tensor placement, not machine capability
- `false` for an undefined `Series`; `noexcept`, metadata-only

## See Also

- [`Series::is_cpu`](is_cpu.md): the exact complement.
- [`Series::is_cuda`](is_cuda.md) / [`Series::is_mps`](is_mps.md): the strict
  per-backend tests below `is_gpu()`.
- `Series::platform()`: the underlying `Platform` value (`cpu` / `cuda` /
  `mps`).
- [`Series::to`](to.md): move a `Series` between platforms.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_floating_point.html">
    <span class="label">Previous</span>
    <span class="title">is_floating_point</span>
  </a>
  <a class="next-link" href="is_leaf.html">
    <span class="label">Next</span>
    <span class="title">is_leaf</span>
  </a>
</div>
