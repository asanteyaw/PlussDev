<!-- pluss-hand-authored -->

# Series::dtype

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
DType Series::dtype() const
```

## What It Does

Returns compact metadata describing how each value in the `Series` is
stored.

The returned `DType` is a small, copyable, allocation-free struct with three
public fields:

- **`scalar`** — the `ScalarType` enum value (`Float32`, `Int64`, `Bool`,
  `Complex64`, ...)
- **`itemsize`** — bytes occupied by **one** stored value
- **`name`** — the stable printable name (`"float32"`, `"int64"`, ...)

and three category queries:

- **`is_floating()`** — any real floating format (`Float16`, `BFloat16`,
  `Float32`, `Float64`, and the compact 8-bit float formats)
- **`is_complex()`** — `Complex64` or `Complex128`
- **`is_integral()`** — any signed or unsigned fixed-width integer
  (`Int8`...`Int64`, `UInt8`...`UInt64`) — notably **not** `Bool`; see the
  gotcha below

The dtype describes the whole `Series` at once — there is no per-element
representation to inspect. `Series` also exposes the three category queries
directly as methods
([`is_floating_point()`](is_floating_point.md), [`is_complex()`](is_complex.md),
[`is_signed()`](is_signed.md)), which are the more direct spelling when a
boolean branch is all you need and `dtype()`'s other fields are not.

## Parameters

None.

## Returns

`DType`
    A value-type struct (`scalar`, `itemsize`, `name`, and the three category
    predicates). Inspecting it never allocates, converts, or copies tensor
    data — it reads metadata only.

## Gradient

Not applicable — `dtype()` returns a plain struct, not a `Series`.

## Examples

Example 1. The three fields and the three category queries, across a real,
an integer, and a complex dtype.

```cpp
auto real = pluss::series::zeros({2}, pluss::ScalarType::Float32);
auto integer = pluss::series::zeros({2}, pluss::ScalarType::UInt32);
auto complex = pluss::series::zeros({2}, pluss::ScalarType::Complex64);
std::cout << "real:    dtype=" << real.dtype() << " itemsize=" << real.dtype().itemsize
          << " floating=" << real.dtype().is_floating() << "\n";
std::cout << "integer: dtype=" << integer.dtype() << " itemsize=" << integer.dtype().itemsize
          << " integral=" << integer.dtype().is_integral() << "\n";
std::cout << "complex: dtype=" << complex.dtype() << " itemsize=" << complex.dtype().itemsize
          << " complex=" << complex.dtype().is_complex() << "\n";
```

### Output

```text
real:    dtype=float32 itemsize=4 floating=true
integer: dtype=uint32 itemsize=4 integral=true
complex: dtype=complex64 itemsize=8 complex=true
```

`std::cout << dtype()` prints `name` directly. `Complex64` costs `8` bytes —
two packed `float32` lanes for the real and imaginary parts.

Example 2. `Bool` is neither floating, complex, nor integral under this
classification — it is its own category.

```cpp
auto b = pluss::series::zeros({2}, pluss::ScalarType::Bool);
std::cout << "bool: floating=" << b.dtype().is_floating()
          << " complex=" << b.dtype().is_complex()
          << " integral=" << b.dtype().is_integral() << "\n";
```

### Output

```text
bool: floating=false complex=false integral=false
```

A routine that branches on "floating vs. integral" and forgets this will
silently mishandle boolean tensors — test dtype `== ScalarType::Bool`
explicitly, or use [`is_signed()`](is_signed.md) style checks the way the
rest of `Series` does.

Example 3. `itemsize` scales with precision inside the same category.

```cpp
auto f16 = pluss::series::zeros({1}, pluss::ScalarType::Float16);
auto f32 = pluss::series::zeros({1}, pluss::ScalarType::Float32);
auto f64 = pluss::series::zeros({1}, pluss::ScalarType::Float64);
std::cout << "float16 itemsize=" << f16.dtype().itemsize << "\n";
std::cout << "float32 itemsize=" << f32.dtype().itemsize << "\n";
std::cout << "float64 itemsize=" << f64.dtype().itemsize << "\n";
```

### Output

```text
float16 itemsize=2
float32 itemsize=4
float64 itemsize=8
```

All three are `is_floating() == true`; only `itemsize` (and precision)
differs. This is what makes `numel() * dtype().itemsize` the right formula
for a byte budget — see [`numel`](numel.md) Example 5.

Example 4. `to()` changes dtype by producing a new tensor; it never mutates
`dtype()` on the original.

```cpp
auto x = pluss::series::zeros({2}, pluss::ScalarType::Float32);
auto y = x.to(pluss::ScalarType::Int64);
std::cout << "x dtype=" << x.dtype() << "  (unchanged)\n";
std::cout << "y dtype=" << y.dtype() << "\n";
```

### Output

```text
x dtype=float32  (unchanged)
y dtype=int64
```

`dtype()` is a read of the current state; converting representation is
[`to`](to.md)'s job, and it is always out-of-place unless you reassign.

## Choosing The Right Query

Use `dtype().scalar` (or compare `dtype()` against another `DType`) when
dispatching on one *exact* representation. Use a category predicate —
`is_floating()`, `is_complex()`, `is_integral()`, or the `Series`-level
[`is_floating_point()`](is_floating_point.md) / [`is_complex()`](is_complex.md) /
[`is_signed()`](is_signed.md) — when several representations are equally
acceptable. A routine meant to accept "any floating input" should not
enumerate `Float32` and `Float64` by hand and miss `Float16`, `BFloat16`, or
the 8-bit float formats.

## Gotchas And Failure Modes

- `itemsize` is bytes **per value**, not the tensor's total storage size.
  Multiply by [`numel`](numel.md) for that.
- `is_integral()` excludes `Bool` — a boolean tensor is `is_floating() ==
  false`, `is_complex() == false`, **and** `is_integral() == false` all at
  once. See Example 2.
- Complex dtypes report `is_complex() == true` but are not classified as
  floating-point, even though their components are floats. Check
  `is_complex()` separately whenever both real and complex inputs are
  accepted.
- Inspecting `dtype()` never converts or copies tensor data — only
  [`to()`](to.md) does that.

## Summary

`dtype()` returns a `Series`' storage metadata as a small `DType` struct:

- `scalar`, `itemsize`, `name`, plus `is_floating()` / `is_complex()` /
  `is_integral()` category queries
- `Bool` belongs to none of the three categories
- read-only and allocation-free; [`to()`](to.md) is what actually converts
- the `Series`-level `is_floating_point()` / `is_complex()` / `is_signed()`
  are the direct spelling for a plain boolean branch

## See Also

- [`Series::is_floating_point`](is_floating_point.md),
  [`Series::is_complex`](is_complex.md),
  [`Series::is_signed`](is_signed.md): the direct `Series`-level category
  predicates.
- [`Series::to`](to.md): convert a `Series` to a different dtype.
- [`Series::numel`](numel.md): pair with `dtype().itemsize` for a total byte
  count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dsplit.html">
    <span class="label">Previous</span>
    <span class="title">dsplit</span>
  </a>
  <a class="next-link" href="eq.html">
    <span class="label">Next</span>
    <span class="title">eq</span>
  </a>
</div>
