<!-- pluss-hand-authored -->
# Note::is_float64

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::is_float64() const noexcept
```

## Recognize Native Floating-Point Notes

`is_float64()` reports whether the receiver stores IEEE 754 double-precision
lanes. It is a dtype query, not a value classifier: an Int64 Note is not
Float64 even when every integer is exactly representable as a `double`, and a
Text Note containing `"0.25"` is still Text.

Natural-language scores and other continuous Note results use Float64 storage.
After the check succeeds, `reals()` exposes a borrowed contiguous
`std::span<const double>` without conversion or allocation. Rank, empty shape,
finite values, infinities, and NaNs do not change the answer. Undefined returns
`false`; the method is constant-time and `noexcept`.

## Example 1: Float64 Is Distinct From Int64

```cpp
const auto scores = pluss::note::make(std::vector<double>{0.25, 0.75});
const auto integers = pluss::note::make(std::vector<std::int64_t>{0, 1});
std::cout << std::boolalpha
          << "scores=" << scores.is_float64()
          << " integers=" << integers.is_float64() << '\n';
```

### Output

```text
scores=true integers=false
```

## Example 2: Absence Is Not A Dtype

```cpp
std::cout << std::boolalpha
          << "absent=" << pluss::Note::undefined().is_float64() << '\n';
```

### Output

```text
absent=false
```

## Example 3: The Check Does Not Read Values

```cpp
const auto values = pluss::note::make(
    std::vector<double>{0.0, std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::quiet_NaN()});
std::cout << std::boolalpha << values.is_float64()
          << " lanes=" << values.numel() << '\n';
```

### Output

```text
true lanes=3
```

## See Also

- [`Note::dtype`](dtype.md): obtain `NoteDType::Float64` directly.
- `Note::reals()`: borrow the native double lanes after checking the dtype.
- [`Note::is_int64`](is_int64.md): the integral numeric dtype.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_digit.html"><span class="label">Previous</span><span class="title">is_digit</span></a>
  <a class="next-link" href="is_int64.html"><span class="label">Next</span><span class="title">is_int64</span></a>
</div>
