<!-- pluss-hand-authored -->

# Series::expand_as

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::expand_as(const Series& other) const -> Series
```

## What It Does

`expand_as()` is [`expand()`](expand.md) with the target shape taken from
another series:

```
x.expand_as(other)  ==  x.expand(other.sizes())
```

It returns a zero-copy broadcast view of the receiver at `other`'s shape. Only
`other`'s *shape* is used — its values, dtype, and storage are irrelevant.

The same rules apply as for [`expand()`](expand.md): size-`1` axes stretch (with
stride `0`), added leading axes are allowed, and a non-size-`1` mismatch is an
error.

## Parameters

**other** (`const Series&`)

The series whose shape to match. Nothing else about it is read.

## Returns

`Series`
    A broadcast view of the receiver with `other`'s shape. Same dtype and
    platform as the receiver.

## Gradient

`expand_as()` is differentiable. As with `expand()`, the backward sums the
upstream gradient over the stretched and added axes.

## Examples

Example 1. A `3 x 1` column expanded to match a `3 x 4` reference.

```cpp
auto col = pluss::make_series({1.0, 2.0, 3.0}).unsqueeze(1);
auto reference = pluss::make_series({
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0},
});

std::cout << col.expand_as(reference);
```

### Output

```text
pluss::Series(shape=[3,4], strides=[1,0], dtype=float64, platform=cpu, numel=12, contiguous=false)
data=[[1, 1, 1, 1],
  [2, 2, 2, 2],
  [3, 3, 3, 3]]
```

Example 2. The backward sums over the axis that was stretched.

```cpp
auto col = pluss::make_series({1.0, 2.0, 3.0}).unsqueeze(1);
col.tracks_slope_(true);
auto reference = pluss::make_series({
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0},
});

col.expand_as(reference).sum().reverse();

std::cout << col.sensitivity();
```

### Output

```text
pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[4],
  [4],
  [4]]
```

## Gotchas And Failure Modes

- Only `other`'s shape matters; its dtype and values are ignored. The result
  keeps the *receiver's* dtype.
- The receiver's shape must be broadcast-compatible with `other`'s, or the call
  raises (message names `expand`).
- The result is a non-contiguous stride-`0` view.

## Summary

`x.expand_as(other)` is `x.expand(other.sizes())`:

- zero-copy broadcast view at another series' shape
- keeps the receiver's dtype; `other`'s values are unused
- differentiable, backward sums over the stretched axes

## See Also

- [`Series::expand`](expand.md): the same, with an explicit shape argument.
- [`Series::broadcast_to`](broadcast_to.md): expand to an explicit shape rather
  than to another series.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="expand.html">
    <span class="label">Previous</span>
    <span class="title">expand</span>
  </a>
  <a class="next-link" href="expm1.html">
    <span class="label">Next</span>
    <span class="title">expm1</span>
  </a>
</div>
