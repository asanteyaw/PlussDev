<!-- pluss-hand-authored -->

# Series::unsqueeze

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::unsqueeze(std::int64_t dim) const
Series& Series::unsqueeze_(std::int64_t dim) const
```

## What It Does

Inserts a size-`1` axis at `dim` and returns the result as a view.

`unsqueeze()` is one of the most important shape-preparation tools in the whole
`Series` API because it changes rank without changing values or `numel()`.

That makes it ideal for:

- broadcast preparation
- explicit batch axis insertion
- explicit feature axis insertion
- making later axis-oriented code line up with the mental model you want

The central point is simple:

- `unsqueeze()` adds structure
- it does not add data

## Parameters

**dim** (`std::int64_t`)

Where to insert the new axis.

For a rank-2 series, the legal insertion positions after normalization are:

- `0`: before both existing axes
- `1`: between them
- `2` or `-1`: after them

So `dim` is not "which existing axis should become bigger." It is "where should
the new size-1 axis live in the final rank."

That is the mental model that prevents most off-by-one mistakes.

## Returns

`Series`
    A view with one more axis, where the inserted axis has extent `1`.

Because this is a view:

- no values are copied
- no values are repeated
- only shape and stride metadata change

## Gradient

`unsqueeze()` is differentiable.

Backward simply removes the inserted axis from the outgoing sensitivity because
the forward pass never duplicated or destroyed any values. It only changed how
the same values are indexed.

## In-Place Form

`unsqueeze_()` is the in-place sibling of [`unsqueeze()`](unsqueeze.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `unsqueeze_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
x.unsqueeze_(0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1, 2, 3]]
```

## Verified Examples

Example 1. Insert a new leading axis.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});
std::cout << mat.unsqueeze(0);
```

### Output

```text
pluss::Series(shape=[1,2,3], strides=[6,3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[[1, 2, 3],
  [4, 5, 6]]]
```

This is the common "wrap the whole tensor in a batch-like axis" form.

Example 2. Insert the axis in the middle.

```cpp
std::cout << mat.unsqueeze(1);
```

### Output

```text
pluss::Series(shape=[2,1,3], strides=[3,3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[[1, 2, 3]],
  [[4, 5, 6]]]
```

This is often the most useful form for broadcast alignment because it separates
existing outer lanes from existing inner lanes.

Example 3. Negative indexing can append the new axis at the end.

```cpp
std::cout << mat.unsqueeze(-1);
```

### Output

```text
pluss::Series(shape=[2,3,1], strides=[3,1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[[1],
  [2],
  [3]],
  [[4],
  [5],
  [6]]]
```

This is the common "turn each scalar feature into a length-1 feature axis"
shape edit.

Example 4. Scalars can also gain rank explicitly.

```cpp
auto scalar = pluss::make_series(42.0);
std::cout << scalar.unsqueeze(0);
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[42]
```

That example matters because it shows the shape distinction clearly:

- scalar shape: `[]`
- length-1 vector shape after unsqueeze: `[1]`

## Notes

- `unsqueeze()` inserts exactly one axis.
- The inserted axis always has size `1`.
- It is a structural view operation, not a numerical transformation.
- Use it when later code wants an explicit axis that your current tensor does
  not yet expose.

## See Also

- [`Series::squeeze`](squeeze.md): remove size-`1` axes.
- [`Series::permute`](permute.md): reorder axes after rank editing.
- [`Series::sizes`](sizes.md): inspect the resulting shape immediately.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="permute.html">
    <span class="label">Previous</span>
    <span class="title">permute</span>
  </a>
  <a class="next-link" href="squeeze.html">
    <span class="label">Next</span>
    <span class="title">squeeze</span>
  </a>
</div>
