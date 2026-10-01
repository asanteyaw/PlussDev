<!-- pluss-hand-authored -->

# Duration::milliseconds

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::milliseconds() const
```

## The Millisecond Field

`milliseconds()` extracts the three decimal digits immediately below the whole
second. Its valid range is `0..999`. Finer precision remains available through
`microseconds()` and `nanoseconds()`; the fields are separate, so a fractional
part of `0.005006007` seconds decomposes to `5` milliseconds, `6`
microseconds, and `7` nanoseconds.

The method does not report total elapsed milliseconds. For example, a span of
one second plus five milliseconds has component `5`, while
`total().milliseconds()` returns `1005`. Negative values use the canonical
nonnegative remainder, making `-1 ns` produce millisecond field `999`.

## Result Contract

The output is a newly allocated, shape-preserving `Int64` Series. `NaT` lanes
remain the exact `Duration::kNaT` sentinel. Extraction is O(`numel`), requires
no floating-point conversion, and therefore preserves all nanosecond identity.

## Examples

Example 1. Isolate the millisecond digits from a full-precision scalar.

```cpp
std::cout << pluss::series::print(
    pluss::duration::nanos(93'784'005'006'007LL).milliseconds());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=5
```

Example 2. Observe normalization at negative boundaries.

```cpp
constexpr std::int64_t hour = 3'600'000'000'000LL;
constexpr std::int64_t day = 86'400'000'000'000LL;
std::cout << pluss::series::print(
    pluss::duration::nanos({-1, -hour, -day}).milliseconds());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[999, 0, 0]
```

Example 3. Matrix structure and absence survive extraction.

```cpp
auto values = pluss::duration::nanos(
    {93'784'005'006'007LL, pluss::Duration::kNaT, -1, 0}, {2,2});
std::cout << pluss::series::print(values.milliseconds());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡   5 -9223372036854775808 ⎤
⎣ 999                    0 ⎦
```

Example 4. No lanes are invented for an empty shape.

```cpp
std::cout << pluss::series::print(
    pluss::duration::zeros({2,0,3}).milliseconds());
```

### Output

```text
Series(shape=[2,0,3], strides=[0,3,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

## See Also

- [`microseconds()`](microseconds.md): the next three fractional digits.
- [`nanoseconds()`](nanoseconds.md): the final three fractional digits.
- [`total().milliseconds()`](total_milliseconds.md): complete elapsed milliseconds.
