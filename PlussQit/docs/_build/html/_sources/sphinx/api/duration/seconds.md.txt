<!-- pluss-hand-authored -->

# Duration::seconds

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::seconds() const
```

## The Within-Minute Second Field

`seconds()` returns the normalized whole-second field within the current
minute. Valid lanes are therefore in `0..59`; fractional-second information is
not rounded or discarded from the Duration, but appears separately through
`milliseconds()`, `microseconds()`, and `nanoseconds()`.

This is a decomposition query, not a unit conversion. A 90-second span has
second field `30`, while `total().seconds()` returns `90`. Negative partial
minutes normalize against the previous minute, so `-1 ns` has second field
`59` rather than `0` or `-1`.

## Result Contract

The method returns a new `Int64` Series at the original shape. It scans each
valid lane once, preserves `NaT` as `Duration::kNaT`, and performs no mutation.
Scalar and empty N-D inputs retain their exact structural identity.

## Examples

Example 1. Extract second `4` without losing the subsecond fields.

```cpp
auto value = pluss::duration::nanos(93'784'005'006'007LL);
std::cout << pluss::series::print(value.seconds());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=4
```

Example 2. Negative values use a positive within-minute remainder.

```cpp
constexpr std::int64_t hour = 3'600'000'000'000LL;
constexpr std::int64_t day = 86'400'000'000'000LL;
std::cout << pluss::series::print(
    pluss::duration::nanos({-1, -hour, -day}).seconds());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[59, 0, 0]
```

Example 3. N-D extraction preserves both shape and the `NaT` lane.

```cpp
auto values = pluss::duration::nanos(
    {93'784'005'006'007LL, pluss::Duration::kNaT, -1, 0}, {2,2});
std::cout << pluss::series::print(values.seconds());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡  4 -9223372036854775808 ⎤
⎣ 59                    0 ⎦
```

Example 4. A zero-element shape is returned unchanged.

```cpp
std::cout << pluss::series::print(
    pluss::duration::zeros({2,0,3}).seconds());
```

### Output

```text
Series(shape=[2,0,3], strides=[0,3,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

## See Also

- [`total().seconds()`](total_seconds.md): complete elapsed seconds, including fractions.
- [`milliseconds()`](milliseconds.md): the first fractional-second field.
- [`minutes()`](minutes.md): the containing field.
