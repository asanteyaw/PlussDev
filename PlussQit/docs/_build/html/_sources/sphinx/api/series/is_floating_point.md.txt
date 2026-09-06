<!-- pluss-hand-authored -->

# Series::is_floating_point

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_floating_point() const
```

## What It Does

Returns `true` when the `Series`' dtype is a **real** floating-point format —
`Float16`, `BFloat16`, `Float32`, `Float64`, or one of the two 8-bit float
formats — and `false` for integers, `Bool`, and **complex**.

It is a dtype test that reads [`dtype`](dtype.md)`().is_floating()` and
nothing else. The *content* of the tensor is irrelevant: a `Float64` tensor
full of `NaN` and `inf` is still `is_floating_point() == true`, because the
storage format is what is being asked about, not the values in it.

The one boundary that surprises people is complex. `Complex64` is a pair of
`Float32` lanes and `Complex128` a pair of `Float64` lanes — the components
*are* floats — but the scalar **category** is complex, so
`is_floating_point()` returns `false` for them. The correct test for "any
continuous real-or-complex numeric data" is therefore:

```cpp
x.is_floating_point() || x.is_complex()
```

`is_floating_point()` alone silently drops complex input.

Across every scalar type, the three category predicates behave as:

| dtype | `is_complex` | `is_floating_point` | `is_signed` |
|---|---|---|---|
| `Bool` | false | false | false |
| `UInt8` … `UInt64` | false | false | false |
| `Int8` … `Int64` | false | false | true |
| `Float16`, `BFloat16`, `Float32`, `Float64`, the 8-bit floats | false | **true** | true |
| `Complex64`, `Complex128` | true | **false** | true |

## Parameters

None.

## Returns

`bool`
    `true` for the real floating formats, `false` for integer, `Bool`, and
    complex dtypes.

## Gradient

Not applicable — `is_floating_point()` returns a plain `bool`.

## Examples

Example 1. Only the real float formats answer `true` — integers, `Bool`, and
complex all answer `false`.

```cpp
for (auto st : {pluss::ScalarType::Bool, pluss::ScalarType::Int32,
                pluss::ScalarType::Float16, pluss::ScalarType::Float32,
                pluss::ScalarType::Float64, pluss::ScalarType::Complex64}) {
  auto x = pluss::series::zeros({1}, st);
  std::cout << x.dtype() << " -> is_floating_point=" << std::boolalpha
            << x.is_floating_point() << "\n";
}
```

### Output

```text
bool -> is_floating_point=false
int32 -> is_floating_point=false
float16 -> is_floating_point=true
float32 -> is_floating_point=true
float64 -> is_floating_point=true
complex64 -> is_floating_point=false
```

`Float16` and `BFloat16` both count — a routine that hand-checks only
`Float32`/`Float64` would wrongly reject half-precision input, which is
exactly the mistake `is_floating_point()` exists to prevent.

Example 2. Complex is excluded even though its components are floats; the
`|| is_complex()` idiom brings it back in.

```cpp
auto f = pluss::series::zeros({1}, pluss::ScalarType::Float64);
auto c = pluss::series::zeros({1}, pluss::ScalarType::Complex128);
auto i = pluss::series::zeros({1}, pluss::ScalarType::Int64);
std::cout << std::boolalpha
          << "float64  continuous? " << (f.is_floating_point() || f.is_complex()) << "\n"
          << "complex128 continuous? " << (c.is_floating_point() || c.is_complex()) << "\n"
          << "int64    continuous? " << (i.is_floating_point() || i.is_complex()) << "\n";
```

### Output

```text
float64  continuous? true
complex128 continuous? true
int64    continuous? false
```

Example 3. Content never changes the answer — a tensor of `NaN` and `inf` is
still floating-point.

```cpp
auto x = pluss::make_series({std::nan(""),
                             std::numeric_limits<double>::infinity(),
                             0.0});
std::cout << "has NaN and inf, is_floating_point=" << std::boolalpha
          << x.is_floating_point() << "\n";
```

### Output

```text
has NaN and inf, is_floating_point=true
```

To ask about the *values*, use [`isnan`](isnan.md), [`isinf`](isinf.md), or
[`isfinite`](isfinite.md); `is_floating_point()` is purely about the format.

Example 4. An integer literal builds an integer `Series`;
`is_floating_point()` reports that, and [`to`](to.md) is what changes it.

```cpp
auto i = pluss::make_series({1, 2, 3});
std::cout << "i.dtype()=" << i.dtype()
          << " is_floating_point=" << std::boolalpha << i.is_floating_point() << "\n";
auto f = i.to(pluss::ScalarType::Float64);
std::cout << "after to(Float64): is_floating_point=" << f.is_floating_point() << "\n";
```

### Output

```text
i.dtype()=int64 is_floating_point=false
after to(Float64): is_floating_point=true
```

Reverse-mode autograd needs a floating dtype — `tracks_slope_` on an integer
`Series` has nothing to accumulate into. `is_floating_point()` is the guard
before enabling gradient tracking on data of unknown provenance.

## Gotchas And Failure Modes

- **Complex is not floating-point.** Use
  `is_floating_point() || is_complex()` whenever complex input should also be
  accepted.
- `Bool` is neither floating, complex, nor integral (see
  [`dtype`](dtype.md)) — a "floating vs. integral" branch has a third case it
  must not forget.
- The answer is format-only. `NaN`/`inf`/`0` content is irrelevant; use the
  `is*` value predicates for that.
- Half formats (`Float16`, `BFloat16`) and the 8-bit floats all count —
  don't reimplement this check by enumerating dtypes by hand.

## Summary

`is_floating_point()` is a dtype-category predicate:

- `true` for the real float formats (`Float16` through `Float64` and the
  8-bit floats), `false` for integer, `Bool`, and complex
- metadata-only — `NaN`/`inf` content does not change it
- complex is excluded despite having float components; add `|| is_complex()`
  for an "any continuous data" test
- the standard guard before enabling reverse-mode autograd

## See Also

- [`Series::dtype`](dtype.md): the exact representation and the same category
  queries on the `DType` struct.
- [`Series::is_complex`](is_complex.md) /
  [`Series::is_signed`](is_signed.md): the other two category predicates.
- [`Series::isnan`](isnan.md) / [`Series::isinf`](isinf.md) /
  [`Series::isfinite`](isfinite.md): the value-level questions
  `is_floating_point()` does *not* answer.
- [`Series::to`](to.md): convert to a floating dtype.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_cuda.html">
    <span class="label">Previous</span>
    <span class="title">is_cuda</span>
  </a>
  <a class="next-link" href="is_gpu.html">
    <span class="label">Next</span>
    <span class="title">is_gpu</span>
  </a>
</div>
