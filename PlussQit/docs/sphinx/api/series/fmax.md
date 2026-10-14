<!-- pluss-hand-authored -->

# Series::fmax

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::fmax(const Series& other) const
```

## Selecting A Maximum Without Letting Missing Values Win

`fmax()` compares two series element by element and returns the larger value.
Its defining rule is what happens when one candidate is `NaN`: the numeric
candidate wins. A result is `NaN` only when both candidates are `NaN`.

This makes `fmax()` useful when `NaN` means "no observation" rather than "the
whole calculation is invalid." For example, two sensors may cover different
parts of a grid. `left.fmax(right)` keeps the strongest available reading at
each location without requiring a separate missing-value mask.

This is a pairwise transform, not a reduction. It does not find one maximum
for the whole receiver, and it does not remove dimensions. The result has the
broadcast shape of the two operands.

## Choosing Between `fmax()` And `maximum()`

Use `fmax()` when a lone `NaN` should behave like a missing candidate. Use
[`maximum()`](maximum.md) when any `NaN` should mark the result invalid. On
integer data, and on floating data without `NaN`, the two methods agree.

## Parameters

**other** (`const Series&`): the competing values. Trailing dimensions follow
the normal broadcasting rules. The two series must be on the same platform.
Complex inputs are rejected because complex numbers have no total ordering.

## Returns

A new `Series` with the broadcast shape and promoted dtype. Neither input is
modified. There is no public in-place `fmax_()` member.

## Gradient

Sensitivity follows the value selected in the forward pass. The larger finite
operand receives the upstream sensitivity and the smaller receives zero. A
finite tie goes entirely to the left operand (`self`), which is how libtorch
routes it. If exactly one operand is `NaN`, the numeric operand receives all of
the sensitivity.

## Examples

### 1. Missing values lose the comparison

```cpp
const double nan = std::numeric_limits<double>::quiet_NaN();
auto left  = pluss::make_series({1.0, nan, 4.0, nan});
auto right = pluss::make_series({2.0, 3.0, 4.0, nan});

pluss::series::print(std::cout, left.fmax(right));
```

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.0000, 3.0000, 4.0000, nan]
```

The second lane takes `3` because it is the only numeric candidate. The final
lane remains `NaN` because neither side supplies a number.

### 2. Reverse routing includes ties and missing lanes

```cpp
const double nan = std::numeric_limits<double>::quiet_NaN();
auto a = pluss::make_series({1.0, 4.0, 3.0, nan});
auto b = pluss::make_series({2.0, 4.0, nan, 5.0});
a.tracks_slope_(true);
b.tracks_slope_(true);

a.fmax(b).sum().reverse();
pluss::series::print(std::cout, a.sensitivity()) << '\n';
pluss::series::print(std::cout, b.sensitivity());
```

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 1.0000, 1.0000, 0.0000]
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 0.0000, 0.0000, 1.0000]
```

Read the columns vertically: `b` wins lane 0; lane 1 is tied and the whole
sensitivity goes to `a`; `a` is the only number in lane 2; and `b` is the only
number in lane 3.

## Gotchas And Failure Modes

- Both-`NaN` still produces `NaN`; this method skips missing candidates but
  does not invent replacements.
- Infinity is an ordinary ordered value. `+inf` wins a maximum and `-inf`
  loses it.
- Broadcasting can enlarge the result. Inspect the operand shapes when a
  pairwise comparison unexpectedly returns more elements.
- There is no scalar overload. Use a one-element `Series`, or use
  [`clamp_min()`](clamp_min.md) when the intent is a fixed lower bound.

## See Also

- [`Series::fmin`](fmin.md): select the smaller available value.
- [`Series::maximum`](maximum.md): propagate `NaN` instead of skipping it.
- [`Series::amax`](amax.md): reduce one or more dimensions to maxima.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floor_divide.html"><span class="label">Previous</span><span class="title">floor_divide</span></a>
  <a class="next-link" href="fmin.html"><span class="label">Next</span><span class="title">fmin</span></a>
</div>

