<!-- pluss-hand-authored -->

# Series::threshold_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::threshold_(double threshold_val, double value)
```

## Mutating A Lower Region

`threshold_()` is the mutating form of [`threshold()`](threshold.md). Every
lane greater than `threshold_val` is retained; every lane less than or equal
to it is replaced by `value`. The receiver keeps its shape, dtype, platform,
and object identity, and the returned reference is the receiver itself.

Choose this form when the old values are no longer needed and mutation is
compatible with the surrounding ownership and differentiation rules. Prefer
the out-of-place form when the receiver is shared, reused in another branch,
or is a reverse-mode leaf whose original value must remain available.

Floating and integral receivers are supported. Bool and complex receivers are
rejected. Fractional thresholds are compared without truncation even for an
integral receiver; the replacement itself is converted to the receiver dtype.
The strict retention rule means a lane exactly on the threshold is replaced.

## Reverse Mode And Mutation Safety

Legal in-place use records the same piecewise derivative as `threshold()`:
retained lanes pass upstream sensitivity and replaced lanes receive zero.
Mutation safety is checked before data are overwritten. An operation that
would invalidate a required saved value or illegally modify a tracked leaf is
rejected rather than silently corrupting the graph.

## Examples

### 1. Replace lanes in the existing object

```cpp
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
x.threshold_(0.0, 7.0);
pluss::series::print(std::cout, x);
```

#### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[7.0000, 7.0000, 0.5000, 1.0000]
```

Both `-1` and the boundary value `0` are replaced.

### 2. Chain through the returned receiver reference

```cpp
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
auto* before = &x;
auto& returned = x.threshold_(0.0, 7.0);
std::cout << std::boolalpha
          << "same object=" << (&returned == before) << '\n';
```

#### Output

```text
same object=true
```

The reference return permits deliberate fluent mutation without creating a
second Series object.

### 3. Preserve matrix shape while mutating values

```cpp
auto matrix = pluss::make_series({{-2.0, 0.0}, {2.0, 4.0}});
matrix.threshold_(1.0, 10.0);
pluss::series::print(std::cout, matrix);
```

#### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 10.0000 10.0000 ⎤
⎣  2.0000  4.0000 ⎦
```

Shape and metadata are unchanged; only qualifying lanes are overwritten.

## See Also

- [`threshold()`](threshold.md) returns a new Series.
- [`clamp_min_()`](clamp_min.md) and [`clamp_max_()`](clamp_max.md) mutate one-sided numeric bounds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="threshold.html">
    <span class="label">Previous</span>
    <span class="title">threshold</span>
  </a>
  <a class="next-link" href="to.html">
    <span class="label">Next</span>
    <span class="title">to</span>
  </a>
</div>
