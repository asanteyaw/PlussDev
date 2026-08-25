<!-- pluss-hand-authored -->

# pluss::series::eye

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::eye(std::int64_t n,
                   std::int64_t m = -1,
                   ScalarType dtype = ScalarType::Float32) -> Series

pluss::series::eye(std::int64_t n,
                   ScalarType dtype) -> Series

pluss::series::eye(std::int64_t n,
                   Config config) -> Series

pluss::series::eye(std::int64_t n,
                   std::int64_t m,
                   Config config) -> Series
```

## What It Builds

Builds a rank-2 identity-style matrix.

That means:

- diagonal entries are `1`
- every off-diagonal entry is `0`
- the diagonal stops once either axis runs out

So `eye()` is the shape-first constructor you reach for when the diagonal
structure itself matters, not just "a matrix mostly full of zeros."

## Overload Map

**`eye(n, dtype)`**

Build a square `n x n` identity matrix.

**`eye(n, m, dtype)`**

Build a rectangular `n x m` identity-like matrix.

**`eye(..., Config)`**

Same logical construction, but with config-based dtype/platform routing.

One subtle overload convenience matters in real code:

- `eye(3, ScalarType::Float64)` means "square 3x3 float64 identity"
- it does not mean "3 rows, dtype accidentally parsed as column count"

That dedicated square+dtype overload exists specifically to keep that common
call site readable.

## Parameters

**n** (`std::int64_t`)

The row count.

**m** (`std::int64_t`)

The column count for the rectangular overload.

When omitted, the implementation uses the square form.

**dtype** (`ScalarType`)

The destination dtype for the explicit-dtype overloads.

**config** (`Config`)

The config path for dtype/platform routing.

## Returns

`Series`
    A rank-2 series whose main diagonal contains ones and whose remaining
    entries contain zeros.

## Examples

Example 1. The square overload is the ordinary identity-matrix constructor.

```cpp
std::cout << pluss::series::eye(3, pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0],
  [0, 0, 1]]
```

Example 2. The rectangular overload keeps the same diagonal rule and simply
stops at the shorter axis.

```cpp
std::cout << pluss::series::eye(2, 4, pluss::ScalarType::Int32);
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=int32, platform=cpu, numel=8, contiguous=true)
data=[[1, 0, 0, 0],
  [0, 1, 0, 0]]
```

Only the first two diagonal positions exist here because there are only two
rows.

Example 3. Boolean output keeps the same shape rule, but the diagonal becomes
`true`/`false` instead of numeric `1`/`0`.

```cpp
std::cout << pluss::series::eye(3, pluss::ScalarType::Bool);
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[[true, false, false],
  [false, true, false],
  [false, false, true]]
```

Example 4. `eye(1, ...)` is still a rank-2 matrix, not a scalar.

```cpp
std::cout << pluss::series::eye(1, pluss::ScalarType::Float64);
```

### Output

```text
pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[1]]
```

Example 5. The config overload changes dtype policy without changing the
identity layout.

```cpp
std::cout << pluss::series::eye(3, pluss::Config{}.with_dtype(pluss::ScalarType::Int16));
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=int16, platform=cpu, numel=9, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0],
  [0, 0, 1]]
```

## Notes

- `eye()` always returns rank `2`.
- Omitting `m` means "square," not "use some implicit broadcasting rule."
- There is no diagonal-offset overload on this surface; `eye()` is strictly the
  main-diagonal constructor.

## See Also

- [`pluss::series::zeros`](zeros.md): shape-first zero construction without the
  diagonal fill rule.
- [`pluss::series::full`](full.md): shape-first constant construction for
  arbitrary fills.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="full.html">
    <span class="label">Next</span>
    <span class="title">full</span>
  </a>
</div>
