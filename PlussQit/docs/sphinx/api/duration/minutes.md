<!-- pluss-hand-authored -->

# Duration::minutes

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::minutes() const
```

## The Within-Hour Minute Field

`minutes()` extracts the minute field left after complete days and hours are
removed. Valid results are bounded to `0` through `59`. Use it when formatting,
partitioning, or reconstructing a duration; use `total().minutes()` when the
required quantity is the complete elapsed magnitude expressed in minutes.

The field follows normalized floor decomposition for negative spans. `-1 ns`
has minute field `59`, because its canonical form is one nanosecond before day
zero. An exact `-1 hour` has minute field `0`. This is deliberately different
from applying a sign to the components of the absolute magnitude.

## Result Contract

The result is a fresh shape-preserving `Int64` Series. `NaT` becomes
`Duration::kNaT`, valid zero remains `0`, and a zero-element input returns a
defined zero-element output without reading lane storage. Runtime is linear in
the number of lanes.

## Examples

Example 1. Read minute `3` from a composite scalar.

```cpp
std::cout << pluss::series::print(
    pluss::duration::nanos(93'784'005'006'007LL).minutes());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=3
```

Example 2. Compare a sub-day negative boundary with exact hour and day boundaries.

```cpp
constexpr std::int64_t hour = 3'600'000'000'000LL;
constexpr std::int64_t day = 86'400'000'000'000LL;
std::cout << pluss::series::print(
    pluss::duration::nanos({-1, -hour, -day}).minutes());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[59, 0, 0]
```

Example 3. Higher-rank extraction preserves arrangement and absence.

```cpp
auto values = pluss::duration::nanos(
    {93'784'005'006'007LL, pluss::Duration::kNaT, -1, 0}, {2,2});
std::cout << pluss::series::print(values.minutes());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡  3 -9223372036854775808 ⎤
⎣ 59                    0 ⎦
```

Example 4. Empty structure remains rank three.

```cpp
std::cout << pluss::series::print(
    pluss::duration::zeros({2,0,3}).minutes());
```

### Output

```text
Series(shape=[2,0,3], strides=[0,3,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

## See Also

- [`hours()`](hours.md): the containing within-day field.
- [`total().minutes()`](total_minutes.md): complete elapsed minutes.
- [`seconds()`](seconds.md): the next finer component.
