<!-- pluss-hand-authored -->

# Series::to

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::to(ScalarType dst) const -> Series
Series::to(Platform target) const -> Series
Series::to(Platform target, ScalarType dst) const -> Series
```

## What It Does

`to()` converts a `Series` to a requested dtype, a requested platform, or both.

That sounds simple, but three public details matter a lot in real code:

- if you only change dtype, values are cast but shape stays the same
- if you only change platform, dtype stays the same
- if you ask for what you already have, the call preserves the current
  representation rather than changing the visible series contract

There is one more load-bearing behavior: when `to()` does need to materialize a
new result, the returned series is contiguous. So `to()` is not only a
representation change. It can also normalize layout as part of the conversion.

## Overload Map

**`to(ScalarType dst)`**

Change dtype while staying on the current platform.

**`to(Platform target)`**

Move to another platform while keeping the current dtype.

**`to(Platform target, ScalarType dst)`**

Do both in one call. Publicly, this is the "move and cast" overload.

## Parameters

**dst** (`ScalarType`)

The destination dtype.

This changes how values are represented:

- widening conversions can preserve more fractional range
- narrowing conversions can discard precision
- integer destinations truncate fractional inputs toward zero on the current
  verified surface
- `bool` treats zero as `false` and non-zero as `true`

**target** (`Platform`)

The destination platform.

This changes where the series lives. On the current public surface:

- moving to the same platform leaves the visible representation unchanged
- moving to a different platform materializes a new series
- some platform and dtype combinations are rejected, such as `float64` on MPS

## Returns

`Series`
    A `Series` in the requested representation.

If the request changes nothing, the visible result is unchanged. If the request
changes dtype or platform, `to()` returns a materialized result.

## Gradient

`to()` preserves reverse-mode connectivity on the current verified surface.

That means you can cast or move a tracked `Series`, continue computing on the
converted result, and still have sensitivity routed back to the original
source.

The gradient is transported back to the original source shape and represented
in the source's own dtype/platform contract.

## Examples

Example 1. `to(dst)` can widen integer data to a floating dtype without
changing shape.

```cpp
auto ints = pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64);
std::cout << ints.to(pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

Example 2. Converting to `bool` follows zero-versus-non-zero truthiness.

```cpp
auto signed_values = pluss::make_series({0, -2, 5}, pluss::ScalarType::Int64);
std::cout << pluss::series::print(signed_values.to(pluss::ScalarType::Bool));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, true, true]
```

The single-argument `pluss::series::print(series)` form returns a printable
string, which is why this example streams that string into `std::cout`.

Example 3. Asking for the dtype you already have is still a valid call. The
visible dtype and values stay the same.

```cpp
auto same_dtype = pluss::make_series({1.0, 2.0, 3.0});
std::cout << same_dtype.to(pluss::ScalarType::Float64) << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

Example 4. The same idea applies to `to(target)` when the series is already on
that platform.

```cpp
auto same_platform = pluss::make_series({2.0, 4.0, 8.0});
pluss::series::print(std::cout, same_platform.to(pluss::Platform::CPU)) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.0000, 4.0000, 8.0000]
```

If you need an explicit standalone copy boundary, use [`clone()`](clone.md)
instead of relying on a same-representation `to(...)` call.

Example 5. The two-argument overload can move and cast in one call. On the
current verified CPU surface, it also shows integer truncation toward zero.

```cpp
auto decimal = pluss::make_series({1.9, -2.1, 3.5});
pluss::series::print(
    std::cout,
    decimal.to(pluss::Platform::CPU, pluss::ScalarType::Int64));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, -2, 3]
```

Example 6. Casting a non-contiguous view materializes a contiguous result.

```cpp
auto transposed = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
}).transpose(0, 1);

auto casted_view = transposed.to(pluss::ScalarType::Float32);

std::cout << "before=" << (transposed.is_contiguous() ? "true" : "false") << "\n";
std::cout << "after=" << (casted_view.is_contiguous() ? "true" : "false") << "\n";
std::cout << casted_view << "\n";
```

### Output

```text
before=false
after=true
pluss::Series(shape=[3,2], strides=[2,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

Example 7. Reverse-mode sensitivity passes through `to()` back to the original
source.

```cpp
auto cast_grad = pluss::make_series({1.5, 2.5, 3.5});
cast_grad.tracks_slope_(true);
cast_grad.to(pluss::ScalarType::Float32).sum().reverse();

std::cout << cast_grad.sensitivity();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
```

## Gotchas And Failure Modes

- `to()` is a representation method, not an ownership method. If storage
  independence matters, use [`clone()`](clone.md).
- `bool` conversion is truthiness-based, not sign-based.
- Integer destinations truncate fractional values toward zero on the current
  verified surface.
- Converting a non-contiguous view materializes a contiguous result.
- Platform moves can fail if the target runtime is unavailable.
- `float64` on MPS is rejected by the public checks.

## Summary

Use `to()` when representation needs to change.

- `to(dst)` changes dtype
- `to(target)` changes platform
- `to(target, dst)` does both
- same-representation requests leave the visible result unchanged
- real conversions materialize contiguous output
- reverse-mode stays connected on the current verified surface

## See Also

- [`Series::clone`](clone.md): force a distinct copy even when representation
  does not change.
- [`Series::contiguous`](contiguous.md): normalize layout without changing dtype
  or platform.
- [`Series::topk`](topk.md): select a ranked subset after conversion.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="threshold_.html">
    <span class="label">Previous</span>
    <span class="title">threshold_</span>
  </a>
  <a class="next-link" href="topk.html">
    <span class="label">Next</span>
    <span class="title">topk</span>
  </a>
</div>
