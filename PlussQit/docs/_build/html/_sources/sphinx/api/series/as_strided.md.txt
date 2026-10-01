<!-- pluss-hand-authored -->

# Series::as_strided

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::as_strided(IntRange sizes, IntRange strides) const
Series Series::as_strided(IntRange sizes, IntRange strides,
                          std::int64_t storage_offset) const
Series& Series::as_strided_(pluss::msharp::RangeView::Int sizes, pluss::msharp::RangeView::Int strides) const
```

`IntRange` is the public integer-range input accepted by the API, including an
initializer list or `std::vector<std::int64_t>`.

## A View From Storage Geometry

`as_strided()` constructs a view by describing exactly how logical coordinates
map into the receiver's storage. For coordinate `(i0, ..., ik)`, it reads

```text
storage_offset + i0 * strides[0] + ... + ik * strides[k]
```

Sizes, strides, and offsets are measured in **elements**, not bytes. This is
the lowest-level public shape operation: it can express a transpose, cropped
window, zero-stride expansion, or overlapping sliding windows without copying.
Prefer `reshape()`, `transpose()`, `unsqueeze()`, or `expand()` when a named
operation states the intention. Use `as_strided()` when the storage mapping is
itself the contract.

The two-argument overload starts at the receiver's current storage offset. The
three-argument overload supplies an absolute element offset in the underlying
storage. That matters when the receiver is already a view: omitting the offset
keeps the current starting point, whereas an explicit offset can point to a
different region of the shared allocation.

The result aliases the same storage. It preserves dtype and platform but may
be non-contiguous, and multiple logical coordinates may refer to one physical
element. Reading an overlapping view is defined. Writes are alias-sensitive;
prefer a functional scatter operation when several lanes can target one slot.

## Validation

- `sizes` and `strides` must have equal lengths.
- Sizes, strides, and the explicit offset must be non-negative.
- Negative-stride views are not supported.
- The greatest reachable element offset must remain inside the allocation.
- A zero-sized view may begin one element past the allocation because it reads
  no value.

Offset calculations are checked for integer overflow before the view exists.

## Reverse Mode And Overlap

Reverse mode scatters each output sensitivity back to the storage location it
read. Contributions add when output lanes overlap. A 3 by 3 view with strides
`{1, 1}` over five elements therefore yields multiplicities
`[1, 2, 3, 2, 1]` when summed. If the receiver itself aliases storage, the
physical contribution is apportioned across equivalent receiver lanes rather
than counted repeatedly. General `as_strided()` reverse is CPU-only.

## In-Place Form

`as_strided_()` is the in-place sibling of [`as_strided()`](as_strided.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `as_strided_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6});
const std::array<std::int64_t, 2> sizes{2, 3};
const std::array<std::int64_t, 2> strides{3, 1};

x.as_strided_(pluss::msharp::RangeView::Int(sizes.data(), sizes.size()),
              pluss::msharp::RangeView::Int(strides.data(), strides.size()));
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 3],
  [4, 5, 6]]
```

## Examples

Example 1. Strides `{1, 3}` produce a transposed 3 by 2 view without moving
data.

```cpp
const auto x = pluss::series::arange(
    0.0, 6.0, 1.0, pluss::ScalarType::Float64);
pluss::series::print(std::cout, x.as_strided({3, 2}, {1, 3}));
```

### Output

```text
Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=
⎡ 0.0000 3.0000 ⎤
⎢ 1.0000 4.0000 ⎥
⎣ 2.0000 5.0000 ⎦
```

Example 2. Equal strides create overlapping diagonal windows.

```cpp
pluss::series::print(std::cout, x.as_strided({3, 3}, {1, 1}));
```

### Output

```text
Series(shape=[3,3], strides=[1,1], dtype=float64, platform=cpu, numel=9, contiguous=false)
data=
⎡ 0.0000 1.0000 2.0000 ⎤
⎢ 1.0000 2.0000 3.0000 ⎥
⎣ 2.0000 3.0000 4.0000 ⎦
```

Example 3. An explicit offset skips the first storage element.

```cpp
pluss::series::print(std::cout, x.as_strided({2, 2}, {2, 1}, 1));
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 1.0000 2.0000 ⎤
⎣ 3.0000 4.0000 ⎦
```

Example 4. Reverse accumulation exposes overlap multiplicity directly.

```cpp
auto x = pluss::series::arange(
    0.0, 5.0, 1.0, pluss::ScalarType::Float64);
x.tracks_slope_(true);
x.as_strided({3, 3}, {1, 1}).sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 2.0000, 1.0000]
```

Example 5. Rank disagreement is rejected before returning a view.

```cpp
try {
  (void)x.as_strided({2, 2}, {1});
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
as_strided: sizes/strides rank mismatch
```

Example 6. Rank-valid geometry can still exceed the allocation.

```cpp
try {
  (void)x.as_strided({4, 4}, {2, 1});
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
as_strided: requested view exceeds storage bounds
```

## See Also

- [`stride()`](stride.md) and [`strides()`](strides.md) inspect layout.
- [`reshape()`](reshape.md) expresses ordinary shape changes safely.
- [`scatter()`](scatter.md) performs indexed functional writes without exposing
  storage strides.
