<!-- pluss-hand-authored -->

# Duration::hours

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::hours() const
```

## The Within-Day Hour Field

`hours()` extracts the normalized hour field after complete days have been
removed. Its range is always `0` through `23` for every valid lane. It is not
the total number of elapsed hours: 26 hours has component `2`, whereas
`total().hours()` returns `26`.

Negative spans use the same floor-normalized representation as the other
components. Both `-1 ns` and `-1 hour` belong to day `-1`; their hour fields are
therefore `23`. An exact `-1 day` has hour field `0`. No component field carries
a separate sign; the day field and positive within-day remainder together
encode the value unambiguously.

## Result Contract

The method allocates an `Int64` Series with the same rank and extents as the
Duration. It performs one linear pass, accepts every public Duration shape,
preserves zero-element shapes, and writes `Duration::kNaT` at absent lanes.

## Examples

Example 1. Extract hour `2` from `1 day, 02:03:04.005006007`.

```cpp
constexpr std::int64_t value_ns = 93'784'005'006'007LL;
std::cout << pluss::series::print(pluss::duration::nanos(value_ns).hours());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=2
```

Example 2. Negative normalization is bounded rather than sign-mirrored.

```cpp
constexpr std::int64_t hour = 3'600'000'000'000LL;
constexpr std::int64_t day = 86'400'000'000'000LL;
std::cout << pluss::series::print(
    pluss::duration::nanos({-1, -hour, -day}).hours());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[23, 23, 0]
```

Example 3. A matrix remains a matrix and carries `NaT` lane-wise.

```cpp
auto values = pluss::duration::nanos(
    {93'784'005'006'007LL, pluss::Duration::kNaT, -1, 0}, {2,2});
std::cout << pluss::series::print(values.hours());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡  2 -9223372036854775808 ⎤
⎣ 23                    0 ⎦
```

Example 4. Empty rank-three structure is retained.

```cpp
std::cout << pluss::series::print(
    pluss::duration::zeros({2,0,3}).hours());
```

### Output

```text
Series(shape=[2,0,3], strides=[0,3,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

## See Also

- [`days()`](days.md): the floor-based day field that carries direction.
- [`total().hours()`](total_hours.md): convert the complete magnitude to hours.
- [`minutes()`](minutes.md): the next field in the normalized decomposition.
