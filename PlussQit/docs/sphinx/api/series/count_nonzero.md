<!-- pluss-hand-authored -->

# Series::count_nonzero

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::count_nonzero() const
Series Series::count_nonzero(std::int64_t dim) const
```

## What It Does

Counts how many elements are **not exactly zero**.

This is the reduction you reach for whenever a count of "hits" is the answer:
how many predictions crossed a threshold, how many entries survived a mask, how
sparse a weight matrix is, how many observations a row actually has. Because a
boolean series stores `true` as `1` and `false` as `0`,
`mask.count_nonzero()` is simply "how many `true`s", which makes it the natural
partner of every comparison operator.

Two forms:

- **`count_nonzero()`** returns a single `int64` scalar — the count across the
  whole tensor.
- **`count_nonzero(dim)`** counts along one axis and removes it, so a
  `[rows, cols]` input reduced over `dim = 0` gives one count per column.

"Not zero" means exactly that: any value that does not compare equal to `0`.
`NaN` is not equal to `0`, so **`NaN` is counted**. `+/-inf` are counted.
`-0.0` *does* compare equal to `0`, so it is not counted. This differs from
[`nonzero()`](nonzero.md) only in what you get back — `nonzero()` returns the
coordinates, `count_nonzero()` returns how many there are.

## Parameters

**dim** (`std::int64_t`): the axis to reduce. Negative values count from the
end. Omit it to count across the entire tensor.

## Returns

`Series`
    An `int64` count. The no-argument form returns a 0-D scalar. The `dim` form
    returns the input shape with `dim` removed (there is no `keepdim`).

## Gradient

`count_nonzero()` is a discrete tally built on an equality test. It is **not
differentiable** — a reverse pass through it alone raises the
objective-is-disconnected guard.

## Examples

Example 1. Across the whole tensor: three of the six entries are non-zero.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({{0.0, 1.0, 2.0}, {0.0, 0.0, 3.0}}).count_nonzero());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=3
```

Example 2. On the result of a comparison it becomes "how many satisfy the
condition".

```cpp
auto x = pluss::make_series({-2.0, 0.0, 3.0, -1.0, 5.0, 0.0});
pluss::series::print(std::cout, x.gt(0.0).count_nonzero());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=2
```

Two values (`3` and `5`) are greater than `0`.

Example 3. `dim = 0` counts non-zeros down each column.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({
        {0.0, 1.0, 2.0},
        {0.0, 0.0, 3.0},
        {7.0, 0.0, 4.0},
    }).count_nonzero(0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 3]
```

Column `0` has one non-zero (`7`), column `1` has one (`1`), column `2` has
three.

Example 4. `dim = 1` counts along each row instead.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({
        {0.0, 1.0, 2.0},
        {0.0, 0.0, 3.0},
        {7.0, 0.0, 4.0},
    }).count_nonzero(1));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[2, 1, 2]
```

Example 5. `NaN` and `inf` are counted; `-0.0` is not.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({
        std::numeric_limits<double>::quiet_NaN(),
        -0.0,
        0.0,
        1.0,
        std::numeric_limits<double>::infinity(),
    }).count_nonzero());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=3
```

`NaN`, `1.0`, and `inf` count; the two zeros (`-0.0` compares equal to `0.0`)
do not.

Example 6. Integer and boolean inputs work the same way.

```cpp
pluss::series::print(std::cout,
                     pluss::make_series({1, 0, 0, 2, 0, 3}).count_nonzero());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=3
```

Example 7. Negative `dim` counts from the end.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({{0.0, 1.0}, {2.0, 0.0}}).count_nonzero(-1));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

## Gotchas And Failure Modes

- `NaN` counts as non-zero. If missing data is encoded as `NaN`, filter it
  first (e.g. `x.isnan().logical_not()`) or you will over-count.
- Only *exact* zero is excluded. Tiny values like `1e-300` are non-zero; use a
  tolerance comparison (`x.abs().gt(eps)`) if "effectively zero" is what you
  mean.
- The `dim` form removes the axis — there is no `keepdim`. To keep it, follow
  with [`unsqueeze(dim)`](unsqueeze.md).
- To count over several axes, either flatten first
  ([`flatten()`](flatten.md).`count_nonzero()`) or chain reductions.
- Not differentiable.

## Summary

`count_nonzero()` tallies the elements that are not exactly `0`:

- whole-tensor form gives an `int64` scalar; `count_nonzero(dim)` reduces and
  drops one axis
- `mask.count_nonzero()` is "how many `true`s"
- `NaN` and `inf` count; `-0.0` does not
- not differentiable

## See Also

- [`Series::nonzero`](nonzero.md): the *coordinates* of the non-zero elements,
  not just how many.
- [`Series::sum`](sum.md): `mask.to(Int64).sum(dim)` is the same count, and
  keeps a `keepdim` option.
- [`Series::bincount`](bincount.md): tally repeated integer *values*, not
  positions.
- [`Series::any`](any.md) / [`Series::all`](all.md): the yes/no questions
  `count_nonzero() > 0` and `count_nonzero() == numel()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="cos.html">
    <span class="label">Next</span>
    <span class="title">cos</span>
  </a>
</div>
