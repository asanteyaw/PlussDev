<!-- pluss-hand-authored -->

# Series::is_cpu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_cpu() const noexcept
```

## What It Does

Returns `true` when this `Series` currently lives on the **CPU** platform,
`false` when it lives on an accelerator.

It is a metadata read of `platform()` — `platform() ==
Platform::CPU` — and nothing else. It never transfers data, never probes what
backends the machine supports, and cannot throw.

Placement is a property of **each individual `Series`**, decided when the
tensor is created (`zeros`, `make_series`, and the rest build on CPU by
default) and changed only by an explicit [`to(Platform)`](to.md) call. Two
tensors in the same program can sit on different platforms, and most binary
operations require both operands on the *same* one — `is_cpu()` (with its
siblings [`is_cuda`](is_cuda.md), [`is_mps`](is_mps.md),
[`is_gpu`](is_gpu.md)) is how you check before combining or before reaching
for direct host access.

`is_cpu()` answers "where is *this* tensor". The different question "can this
*machine* use backend X" is `pluss::platform_available(p)` — see Example 3.

## Parameters

None.

## Returns

`bool`
    `true` iff `platform() == Platform::CPU`.

## Gradient

Not applicable — `is_cpu()` returns a plain `bool`.

## Examples

Example 1. A freshly built `Series` is on the CPU; the four platform
predicates for it read as one row of a truth table.

```cpp
auto x = pluss::series::zeros({2}, pluss::ScalarType::Float32);
std::cout << std::boolalpha
          << "platform()=" << x.platform()
          << " is_cpu=" << x.is_cpu()
          << " is_gpu=" << x.is_gpu()
          << " is_cuda=" << x.is_cuda()
          << " is_mps=" << x.is_mps() << "\n";
```

### Output

```text
platform()=cpu is_cpu=true is_gpu=false is_cuda=false is_mps=false
```

Exactly one of `is_cpu()` and `is_gpu()` is ever `true` — they partition
every `Series`. `is_cuda()` and `is_mps()` further pin *which* accelerator,
and are both `false` here.

Example 2. `to()` moves the data; the predicates follow it. (Output from an
Apple-Silicon host where MPS is available — a CUDA host or a CPU-only host
reports its own platforms.)

```cpp
auto x = pluss::series::zeros({2});
std::cout << std::boolalpha << "before: is_cpu=" << x.is_cpu() << "\n";
auto m = x.to(pluss::pMPS);
std::cout << "on MPS: is_cpu=" << m.is_cpu() << " is_mps=" << m.is_mps() << "\n";
auto back = m.to(pluss::pCPU);
std::cout << "back:   is_cpu=" << back.is_cpu() << "\n";
```

### Output

```text
before: is_cpu=true
on MPS: is_cpu=false is_mps=true
back:   is_cpu=true
```

`x`, `m`, and `back` are three separate `Series`; `to()` never mutates the
one it is called on.

Example 3. `is_cpu()` vs `pluss::platform_available(pluss::pCPU)` — two
different questions.

```cpp
auto x = pluss::series::zeros({2});
std::cout << std::boolalpha
          << "x.is_cpu()                          = " << x.is_cpu() << "\n"
          << "platform_available(pCPU)  [machine] = " << pluss::platform_available(pluss::pCPU) << "\n"
          << "platform_available(pCUDA) [machine] = " << pluss::platform_available(pluss::pCUDA) << "\n"
          << "platform_available(pMPS)  [machine] = " << pluss::platform_available(pluss::pMPS) << "\n";
```

### Output

```text
x.is_cpu()                          = true
platform_available(pCPU)  [machine] = true
platform_available(pCUDA) [machine] = false
platform_available(pMPS)  [machine] = true
```

`platform_available(pCPU)` is always `true`; the accelerator entries depend
on the host. Use `platform_available` to decide *whether to move*; use
`is_cpu()` to check *where a tensor already is*.

Example 4. An **undefined** `Series` reports `is_cpu() == true` — because
`platform()` defaults to `Platform::CPU` for a tensor with no storage.

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

`is_cpu() == true` therefore does **not** guarantee a usable tensor — check
`defined()` first if the `Series` might be uninitialized.

Example 5. Combining tensors from two platforms raises — the guard
`is_cpu()` helps you place before this can happen.

```cpp
auto a = pluss::series::zeros({2});                 // CPU
auto b = pluss::series::ones({2}).to(pluss::pMPS);  // MPS
try {
  auto c = a.add(b);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
add: expected all Series on same platform, got CPU and MPS
```

The fix is `a.add(b.to(pluss::pCPU))` (or move `a` up to MPS) — decide with
`is_cpu()` / `is_mps()` which direction to converge.

## Gotchas And Failure Modes

- `is_cpu()` is about *this tensor's* current placement, not the machine's
  capabilities. Use `pluss::platform_available(p)` for the latter.
- An undefined `Series` reports `is_cpu() == true`. Pair with
  `defined()` when that matters.
- Placement is per-`Series`. A binary op between a CPU tensor and an
  accelerator tensor raises; move one side with [`to()`](to.md) first.
- The predicate never transfers data and never throws (`noexcept`) — it is
  free to call in a hot path.

## Summary

`is_cpu()` reports whether a `Series` currently resides on the CPU:

- `platform() == Platform::CPU`; the exact complement of
  [`is_gpu`](is_gpu.md)
- per-tensor placement, set at creation, changed only by
  [`to(Platform)`](to.md)
- distinct from `pluss::platform_available(pCPU)` (a machine capability, and
  always `true`)
- `true` for an undefined `Series` — guard with `defined()`

## See Also

- [`Series::is_gpu`](is_gpu.md): the exact negation — "on any accelerator".
- [`Series::is_cuda`](is_cuda.md) / [`Series::is_mps`](is_mps.md): the strict
  per-backend tests.
- `Series::platform()`: the underlying `Platform` value these predicates
  read (`cpu` / `cuda` / `mps`).
- [`Series::to`](to.md): move a `Series` between platforms.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_contiguous.html">
    <span class="label">Previous</span>
    <span class="title">is_contiguous</span>
  </a>
  <a class="next-link" href="is_cuda.html">
    <span class="label">Next</span>
    <span class="title">is_cuda</span>
  </a>
</div>
