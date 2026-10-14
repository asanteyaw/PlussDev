<!-- pluss-hand-authored -->

# Series::fmin

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::fmin(const Series& other) const
```

## Selecting The Smallest Available Candidate

`fmin()` performs an elementwise minimum while treating a lone `NaN` as a
missing candidate. If one side is numeric and the other is `NaN`, the numeric
side is returned. Only a pair of `NaN` values produces a `NaN` result.

Think of it as a merge operation for lower envelopes. If two calculations
produce candidate costs, limits, or earliest valid measurements with gaps in
different positions, `fmin()` keeps the smaller value wherever either source
has one. It does not reduce a dimension; every broadcasted input position has
one output position.

## Parameters

**other** (`const Series&`): the second candidate. It broadcasts against the
receiver, must share its platform, and may have a different real dtype. Complex
inputs are rejected.

## Returns

A new `Series` at the broadcast shape and promoted dtype. The receiver and
`other` remain unchanged. There is no public in-place `fmin_()` member.

## Gradient

The selected finite operand receives the upstream sensitivity. A finite tie
goes entirely to the left operand (`self`), matching libtorch. A `NaN` operand
receives no sensitivity when the other side supplies the result.

## Examples

### 1. Form a lower envelope with gaps

```cpp
const double nan = std::numeric_limits<double>::quiet_NaN();
auto left  = pluss::make_series({1.0, nan, 4.0, nan});
auto right = pluss::make_series({2.0, 3.0, 4.0, nan});

pluss::series::print(std::cout, left.fmin(right));
```

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 3.0000, 4.0000, nan]
```

Lane 0 chooses the smaller number. Lane 1 chooses the only number. Lane 2 is a
finite tie, and lane 3 has no numeric candidate.

### 2. Pair it with `fmax()` to obtain bounds

```cpp
const double nan = std::numeric_limits<double>::quiet_NaN();
auto left  = pluss::make_series({1.0, nan, 4.0, nan});
auto right = pluss::make_series({2.0, 3.0, 4.0, nan});

pluss::series::print(std::cout, left.fmin(right)) << '\n';
pluss::series::print(std::cout, left.fmax(right));
```

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 3.0000, 4.0000, nan]
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.0000, 3.0000, 4.0000, nan]
```

Together these calls produce the lower and upper available candidate at every
position. Where only one source is present, both bounds collapse to it.

## Gotchas And Failure Modes

- `fmin()` skips one `NaN`; [`minimum()`](minimum.md) propagates it. Choose
  deliberately according to what `NaN` means in the dataset.
- Both-`NaN` remains `NaN`.
- There is no scalar overload. A fixed upper limit is more directly expressed
  by [`clamp_max()`](clamp_max.md).
- This is pairwise selection. Use [`amin()`](amin.md) to reduce dimensions.

## See Also

- [`Series::fmax`](fmax.md): select the larger available value.
- [`Series::minimum`](minimum.md): the `NaN`-propagating comparison.
- [`Series::amin`](amin.md): reduce dimensions to minima.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmax.html"><span class="label">Previous</span><span class="title">fmax</span></a>
  <a class="next-link" href="fmod.html"><span class="label">Next</span><span class="title">fmod</span></a>
</div>

