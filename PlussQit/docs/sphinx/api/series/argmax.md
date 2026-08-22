<!-- pluss-hand-authored -->

# Series::argmax

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::argmax() const -> Series
Series::argmax(std::int64_t dim) const -> Series
Series::argmax(std::int64_t dim, bool keepdim) const -> Series
```

## What It Does

Returns the index of the largest value rather than the value itself.

That distinction is the whole point of `argmax()`:

- [`max()`](max.md) answers "what is the winning value?"
- `argmax()` answers "where is the winning value?"

Those are often used together, but they are not interchangeable. `argmax()`
returns `int64` indices, not the source dtype, and its no-argument overload uses
flat index semantics rather than coordinate semantics.

## Overload Map

**`argmax()`**

Finds the largest value across the entire series and returns its flat index in
contiguous row-major order.

**`argmax(dim)`**

Reduces one axis and returns the local winning index inside that axis.

**`argmax(dim, keepdim)`**

Same reduction as `argmax(dim)`, but optionally keeps the reduced axis as size
`1`.

There is no binary `argmax(other)` overload. `argmax()` is purely a reduction
family.

## Parameters

**dim** (`std::int64_t`)

The axis whose local winner you want.

- positive dims count from the front
- negative dims count from the back
- the dim must refer to an existing axis

For `argmax(dim, ...)`, the returned indices are always local to that reduced
axis. They are not global offsets into the full buffer.

**keepdim** (`bool`)

Whether the reduced axis is retained as size `1`.

- `false`: remove the axis
- `true`: keep the axis

This is mainly useful when the index result needs to stay shape-aligned with
another reduced output.

## Returns

`Series`
    An `int64` index series. The output is scalar for `argmax()`, or a reduced
    `int64` series for the dimension overloads.

## Gradient

`argmax()` is an index-producing selection op, not a differentiable value
reduction.

As currently implemented, it does not attach a reverse-mode source link. Treat
it as a control/selection primitive rather than something that participates in a
useful gradient path.

## Tie Policy

Ties go to the first winner encountered in the traversal order.

That means:

- for `argmax()`, ties resolve to the first winning flat position
- for `argmax(dim, ...)`, ties resolve to the first winning position along the
  reduced axis

This is a first-winner selector, not a tie-sharing operation like
[`max()`](max.md).

## Examples

The examples on this page build incrementally.

Example 1. With no arguments, `argmax()` returns a flat global index.

```cpp
auto highs = pluss::make_series({
    {1.0, 7.0, 3.0},
    {9.0, 7.0, 6.0},
});

std::cout << highs.argmax();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=3
```

The flattened row-major order is `[1, 7, 3, 9, 7, 6]`, so the winning `9`
lives at flat index `3`.

Example 2. `argmax(dim)` returns local axis indices instead.

```cpp
std::cout << highs.argmax(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

Row `0` wins at column `1`; row `1` wins at column `0`.

Example 3. `keepdim=true` preserves the reduced axis for later shape alignment.

```cpp
std::cout << highs.argmax(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [0]]
```

Only the shape changes here, not the local winning indices.

Example 4. Negative dims work the same way they do on value reductions.

```cpp
std::cout << highs.argmax(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

For a rank-2 series, `-1` means the last axis, so this matches `argmax(1)`.

Example 5. Ties resolve to the first winning position, not all winners.

```cpp
auto argmax_ties = pluss::make_series({
    {4.0, 9.0, 9.0},
    {2.0, 2.0, 1.0},
});

std::cout << argmax_ties.argmax(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

The first row has equal maxima at columns `1` and `2`, but `1` wins because it
appears first along the reduced axis. The same thing happens in the second row:
the first `2` wins over the second `2`.

Example 6. A scalar has only one legal winning position, so the answer is `0`.

```cpp
auto scalar = pluss::make_series(42.0);

std::cout << scalar.argmax();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

This is the natural scalar convention: a single element is both the first and
only winner.

Example 7. Reducing an empty dimension now fails explicitly instead of silently
returning a meaningless index.

```cpp
auto empty_rows = pluss::series::zeros({0, 3});

try {
  std::cout << empty_rows.argmax(0) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
argmax: cannot reduce an empty dimension
```

That failure mode matters because an index result must point to a real element.
On an empty lane, there is no valid winner to point at.

## Notes

- `argmax()` returns indices, not values.
- The no-argument overload is a full-series flat reduction.
- The returned dtype is `int64`.
- Tie handling is first-winner, not tie-sharing.

## See Also

- [`Series::max`](max.md): the value-returning counterpart.
- [`Series::argmin`](argmin.md): the lower-winner index counterpart.
- [`Series::max(dim, keepdim)`](max.md): same shape rules, but returns values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="argmin.html">
    <span class="label">Next</span>
    <span class="title">argmin</span>
  </a>
</div>
