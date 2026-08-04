<!-- pluss-hand-authored -->

# Series::squeeze

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::squeeze() const -> Series
Series::squeeze(std::int64_t dim) const -> Series
```

## What It Does

Removes size-`1` axes from a series and returns the result as a view.

The zero-argument overload removes every singleton axis. The one-argument
overload targets only one axis, which is the safer choice when you want to be
explicit about exactly which layout change is allowed to happen.

`squeeze()` is the natural counterpart to [`unsqueeze()`](unsqueeze.md): one
makes singleton axes explicit, the other removes them once they are no longer
needed.

## Parameters

**dim** (`std::int64_t`, second overload only)

The specific axis to remove if and only if its size is `1`.

Negative indexing is supported. If the selected axis is not singleton, the
one-argument overload returns `self` unchanged rather than throwing.

## Returns

`Series`
    A view with singleton axes removed.

If every axis is removed, the result is a true scalar `Series` with
`shape=[]`, not a fake length-1 vector.

## Gradient

`squeeze()` is differentiable. Backward reinserts the removed singleton axes by
reshaping the outgoing sensitivity to the original shape.

Like [`unsqueeze()`](unsqueeze.md), this is a pure layout operation: no values
are merged, averaged, or duplicated.

## Examples

Example 1. Remove every singleton axis.

```cpp
auto singleton = pluss::make_series(
    std::vector<double>{10, 20, 30, 40, 50, 60}, {1, 2, 1, 3, 1});
std::cout << singleton.squeeze();
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[10, 20, 30],
  [40, 50, 60]]
```

All size-`1` axes disappear, leaving only the substantive `[2, 3]` layout.

Example 2. Remove one specific singleton axis and leave the rest alone.

```cpp
std::cout << singleton.squeeze(2);
```

### Output

```text
pluss::Series(shape=[1,2,3,1], strides=[6,3,1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[[[10],
  [20],
  [30]],
  [[40],
  [50],
  [60]]]]
```

Only axis `2` was removed. The leading and trailing singleton axes remain,
because this overload is intentionally surgical.

Example 3. Targeting a non-singleton axis is a no-op, not an error.

```cpp
std::cout << singleton.squeeze(1);
```

### Output

```text
pluss::Series(shape=[1,2,1,3,1], strides=[6,3,3,1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[[[[10],
  [20],
  [30]]],
  [[[40],
  [50],
  [60]]]]]
```

Axis `1` has length `2`, so there is nothing to remove. Returning `self`
unchanged makes it safe to call the dimension-specific overload in pipelines
where the axis is expected to be singleton but may already have been normalized.

Example 4. When every axis is singleton, `squeeze()` produces a true scalar.

```cpp
auto all_singletons = pluss::make_series(std::vector<double>{42.0}, {1, 1});
std::cout << all_singletons.squeeze();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=42
```

That scalar shape matters because it keeps `squeeze()` aligned with the rest of
the `Series` surface, which already distinguishes scalars from length-1 vectors.

## Notes

- `squeeze()` is a view op.
- `squeeze(dim)` only removes the selected axis if its length is `1`.
- The zero-arg overload can reduce rank all the way to scalar.

## See Also

- [`Series::unsqueeze`](unsqueeze.md): insert singleton axes.
- [`Series::flatten`](flatten.md): collapse a range of axes into one axis rather
  than simply removing singleton axes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unsqueeze.html">
    <span class="label">Previous</span>
    <span class="title">unsqueeze</span>
  </a>
  <a class="next-link" href="flatten.html">
    <span class="label">Next</span>
    <span class="title">flatten</span>
  </a>
</div>
