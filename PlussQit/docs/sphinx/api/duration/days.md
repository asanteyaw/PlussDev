<!-- pluss-hand-authored -->

# Duration::days

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::days() const
```

## The Day Field

`days()` returns the whole-day field of the Duration's normalized decomposition.
It does not convert the complete elapsed magnitude to days. That distinction is
visible whenever a lane contains a partial day: a span of 26 hours has a day
field of `1`, while `total().days()` returns `1.08333...`.

Normalization uses floor division. Consequently, `days()` is the field that
carries negative direction: `-1 ns` decomposes as `-1` day plus
`23:59:59.999999999`, not as day `0` plus negative subfields. Together, the
seven component methods satisfy:

```text
duration = days*24h + hours*1h + minutes*1min + seconds*1s
         + milliseconds*1ms + microseconds*1us + nanoseconds*1ns
```

Every other component is nonnegative. This representation makes component
tuples canonical and reconstructible for positive and negative spans.

## Result Contract

The result is a newly allocated `Int64` Series with exactly the receiver's
shape. Scalar, N-D, and zero-element shapes remain distinct. A `NaT` lane is
written as `Duration::kNaT`; test the source with `isnat()` when that sentinel
must be masked. The operation is O(`numel`) and does not mutate the Duration.

## Examples

Example 1. Read the day field from a fully populated positive span.

```cpp
constexpr std::int64_t value_ns =
    86'400'000'000'000LL + 2*3'600'000'000'000LL +
    3*60'000'000'000LL + 4*1'000'000'000LL +
    5*1'000'000LL + 6*1'000LL + 7;
std::cout << pluss::series::print(pluss::duration::nanos(value_ns).days());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=1
```

Example 2. Negative partial days floor into the previous day.

```cpp
constexpr std::int64_t hour = 3'600'000'000'000LL;
constexpr std::int64_t day = 86'400'000'000'000LL;
auto values = pluss::duration::nanos({-1, -hour, -day});
std::cout << pluss::series::print(values.days());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[-1, -1, -1]
```

Example 3. Shape and `NaT` positions are preserved.

```cpp
constexpr std::int64_t value_ns = 93'784'005'006'007LL;
auto values = pluss::duration::nanos(
    {value_ns, pluss::Duration::kNaT, -1, 0}, {2,2});
std::cout << pluss::series::print(values.days());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡  1 -9223372036854775808 ⎤
⎣ -1                    0 ⎦
```

Example 4. A zero-element receiver keeps its rank and extents.

```cpp
std::cout << pluss::series::print(
    pluss::duration::zeros({2,0,3}).days());
```

### Output

```text
Series(shape=[2,0,3], strides=[0,3,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

## See Also

- [`total().days()`](total_days.md): convert the entire magnitude to fractional days.
- [`hours()`](hours.md): read the normalized within-day hour field.
- [`isnat()`](isnat.md): identify lanes represented by the sentinel.
