<!-- pluss-hand-authored -->

# Series::round

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::round() const -> Series
Series& Series::round_()
```

## What It Does

`round()` maps each lane to the nearest integer.

The most important behavior to know is the tie rule. On the current verified
surface, half-way values round away from zero:

- `1.5 -> 2`
- `-1.5 -> -2`

That tie rule affects the values you see and also tells you where the method's
discontinuity points matter most.

## Returns

`Series`
    A new `Series` with nearest-integer rounded values.

## Gradient

Between tie and integer boundaries, `round()` is locally constant, so the usual
reverse-mode sensitivity is `0` through those lanes.

## In-Place Form

`round_()` is the in-place sibling of [`round()`](round.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({1.4, 1.5, 2.5});

auto& ref = x.round_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 2]
same object: true
```

## Examples

Example 1. The current tie rule is half away from zero.

```cpp
std::cout << pluss::make_series({-1.6, -1.5, -1.4, 1.4, 1.5, 1.6}).round();
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[-2, -2, -1, 1, 2, 2]
```

Example 2. Reverse-mode sensitivity is zero in the ordinary piecewise-constant
regions.

```cpp
auto x = pluss::make_series({-1.2, 0.0, 3.8});
x.tracks_slope_(true);

auto y = x.round();
y.sum().reverse();

std::cout << "out=" << y << "\n";
std::cout << "dL_dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1, 0, 4]
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

## Gotchas And Failure Modes

- `round()` is not the same as `floor()`, `ceil()`, or `trunc()`.
- Half-way cases matter; the current verified tie rule is away from zero.
- The transform is not smooth.

## Summary

Use `round()` when values should go to the nearest integer and the
away-from-zero tie rule is acceptable for your policy.

## See Also

- [`Series::floor`](floor.md): always downward.
- [`Series::ceil`](ceil.md): always upward.
- [`Series::trunc`](trunc.md): toward-zero removal of the fractional part.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reverse.html">
    <span class="label">Previous</span>
    <span class="title">reverse</span>
  </a>
  <a class="next-link" href="scatter.html">
    <span class="label">Next</span>
    <span class="title">scatter</span>
  </a>
</div>
