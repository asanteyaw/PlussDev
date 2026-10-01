<!-- pluss-hand-authored -->

# Duration::nanoseconds

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::nanoseconds() const
```

## The Sub-Microsecond Field

`nanoseconds()` extracts the final three digits of the normalized decomposition,
so every valid result lies in `0..999`. It is not the Duration's complete
nanosecond count. Use `total().nanoseconds()` for that conversion or `host()`
when an exact owning `std::vector<std::int64_t>` is required.

The distinction is substantial: a value of `1'000'000'007 ns` has nanosecond
component `7`, while its total is `1'000'000'007`. Because this method uses
integer remainder arithmetic, it preserves the final digits exactly even for
magnitudes beyond floating-point integer precision. Floor normalization makes
the component of `-1 ns` equal to `999`, paired with the preceding fields of
`-1 day, 23:59:59.999999`.

## Result Contract

The method returns a new shape-preserving `Int64` Series, maps `NaT` to
`Duration::kNaT`, and supports scalar through rank-three and zero-element
inputs. It is O(`numel`) and does not mutate or reinterpret the source.

## Examples

Example 1. Recover the final three decimal digits exactly.

```cpp
std::cout << pluss::series::print(
    pluss::duration::nanos(93'784'005'006'007LL).nanoseconds());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=7
```

Example 2. The last nanosecond before zero has component `999`.

```cpp
constexpr std::int64_t hour = 3'600'000'000'000LL;
constexpr std::int64_t day = 86'400'000'000'000LL;
std::cout << pluss::series::print(
    pluss::duration::nanos({-1, -hour, -day}).nanoseconds());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[999, 0, 0]
```

Example 3. Matrix structure and absence are retained exactly.

```cpp
auto values = pluss::duration::nanos(
    {93'784'005'006'007LL, pluss::Duration::kNaT, -1, 0}, {2,2});
std::cout << pluss::series::print(values.nanoseconds());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡   7 -9223372036854775808 ⎤
⎣ 999                    0 ⎦
```

Example 4. Empty arrays retain shape without producing lane values.

```cpp
std::cout << pluss::series::print(
    pluss::duration::zeros({2,0,3}).nanoseconds());
```

### Output

```text
Series(shape=[2,0,3], strides=[0,3,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

## See Also

- [`microseconds()`](microseconds.md): the preceding three-digit field.
- [`total().nanoseconds()`](total_nanoseconds.md): the complete elapsed count as Series.
- [`host()`](host.md): exact owning host extraction without a Series conversion.
