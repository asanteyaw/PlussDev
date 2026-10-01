<!-- pluss-hand-authored -->

# Series::strides

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
pluss::msharp::RangeView::Int Series::strides() const
```

## Read The Complete Storage Walk

`strides()` returns one element-stride per axis. If coordinate `i[d]` advances
by one, the storage offset advances by `strides()[d]` elements. Combined with
[`sizes()`](sizes.md), this metadata describes the Series view without copying
its payload.

The returned `RangeView` borrows metadata from the Series implementation. It
is cheap and read-only; do not retain it beyond the lifetime of the Series
whose layout it describes. Use [`stride(dim)`](stride.md) for one normalized
axis, and `strides()` when inspecting or forwarding the complete layout.

Strides are measured in elements, not bytes. Multiply by `dtype().itemsize`
when a byte distance is required. A zero stride denotes broadcasting: several
logical coordinates read the same physical element. A larger-than-packed
stride denotes holes, as in stepped or column slices. Neither property alone
makes a view invalid.

## Examples

### 1. Packed And Transposed Layouts

```cpp
auto table = pluss::make_series({
    {4.0, 1.0, 4.0, 2.0},
    {3.0, 5.0, 0.0, 5.0},
});
auto print = [](const pluss::Series& x) {
  std::cout << "[";
  for (std::size_t i = 0; i < x.strides().size(); ++i) {
    if (i) std::cout << ",";
    std::cout << x.strides()[i];
  }
  std::cout << "]";
};
print(table);
std::cout << "\n";
print(table.transpose(0, 1));
```

### Output

```text
[4,1]
[1,4]
```

Transpose swaps shape and stride metadata; it does not rearrange storage.

### 2. Broadcasting Introduces A Zero Stride

```cpp
auto wide = pluss::make_series({1.0, 2.0, 3.0})
                .unsqueeze(0)
                .expand({4, 3});
std::cout << "strides=";
print(wide);
```

### Output

```text
strides=[0,1]
```

Advancing through the four logical rows does not advance storage.

### 3. Scalars Have No Axis Metadata

```cpp
std::cout << "strides=";
print(pluss::make_series(1.0));
```

### Output

```text
strides=[]
```

### 4. A Column Split Can Be Non-Dense Without Overlap

```cpp
auto first_column = table.split_with_sizes({1, 3}, 1)[0];
std::cout << first_column << "\nstrides=";
print(first_column);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[4,1], dtype=float64, platform=cpu, numel=2, contiguous=false)
data=[[4],
  [3]]
strides=[4,1]
```

The two values are four storage elements apart. The view is valid and
non-overlapping, but not dense.

`strides()` does not allocate or participate in reverse mode. Calling it on an
undefined Series is invalid because there is no metadata to borrow.

## See Also

- [`Series::stride`](stride.md): query one axis, including negative axes.
- [`Series::sizes`](sizes.md): extents paired with these strides.
- [`Series::is_contiguous`](is_contiguous.md): test packed layout.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stride.html"><span class="label">Previous</span><span class="title">stride</span></a>
  <a class="next-link" href="sub.html"><span class="label">Next</span><span class="title">sub</span></a>
</div>
