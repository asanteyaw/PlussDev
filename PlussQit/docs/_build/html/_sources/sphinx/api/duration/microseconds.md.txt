<!-- pluss-hand-authored -->

# Duration::microseconds

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::microseconds() const
```

## The Microsecond Field

`microseconds()` returns the three decimal digits below the millisecond field,
in the range `0..999`. It is the middle subsecond component, not all
microseconds elapsed since the beginning of the second. Thus
`0.005006007` seconds yields millisecond `5`, microsecond `6`, and nanosecond
`7`.

Use `total().microseconds()` for a complete unit conversion. Component
extraction remains integer-exact and never passes through floating point.
For negative durations, floor normalization keeps this field nonnegative:
`-1 ns` is represented with microsecond field `999`.

## Result Contract

The returned `Int64` Series has the receiver's exact shape and owns its storage.
`NaT` maps to `Duration::kNaT`; empty shapes remain defined and empty. The
method performs one O(`numel`) pass and leaves the Duration unchanged.

## Examples

Example 1. Select the microsecond digits from a nanosecond-exact value.

```cpp
std::cout << pluss::series::print(
    pluss::duration::nanos(93'784'005'006'007LL).microseconds());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=6
```

Example 2. The field remains within `0..999` for negative spans.

```cpp
constexpr std::int64_t hour = 3'600'000'000'000LL;
constexpr std::int64_t day = 86'400'000'000'000LL;
std::cout << pluss::series::print(
    pluss::duration::nanos({-1, -hour, -day}).microseconds());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[999, 0, 0]
```

Example 3. N-D shape and `NaT` identity are lane-preserving.

```cpp
auto values = pluss::duration::nanos(
    {93'784'005'006'007LL, pluss::Duration::kNaT, -1, 0}, {2,2});
std::cout << pluss::series::print(values.microseconds());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡   6 -9223372036854775808 ⎤
⎣ 999                    0 ⎦
```

Example 4. Zero cardinality preserves all three axes.

```cpp
std::cout << pluss::series::print(
    pluss::duration::zeros({2,0,3}).microseconds());
```

### Output

```text
Series(shape=[2,0,3], strides=[0,3,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

## See Also

- [`milliseconds()`](milliseconds.md): the preceding fractional field.
- [`nanoseconds()`](nanoseconds.md): the remaining sub-microsecond field.
- [`total().microseconds()`](total_microseconds.md): complete elapsed microseconds.
