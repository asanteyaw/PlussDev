<!-- pluss-hand-authored -->

# Duration Operators

[Back to Duration API Reference](../../duration_api_reference)

## Signatures

```cpp
Duration Duration::operator+(const Duration& other) const;
Duration Duration::operator-(const Duration& other) const;
Duration Duration::operator-() const;
Duration& Duration::operator+=(const Duration& other);
Duration& Duration::operator-=(const Duration& other);

Duration Duration::operator*(std::int64_t count) const;
Duration Duration::operator/(std::int64_t divisor) const;

Series Duration::operator==(const Duration& other) const;
Series Duration::operator!=(const Duration& other) const;
Series Duration::operator<(const Duration& other) const;
Series Duration::operator<=(const Duration& other) const;
Series Duration::operator>(const Duration& other) const;
Series Duration::operator>=(const Duration& other) const;

std::ostream& operator<<(std::ostream& out, const Duration& value);
```

## Reading The Operator Surface

Duration-duration arithmetic combines elapsed values elementwise and applies
ordinary N-D broadcasting. Unary minus reverses direction. Integer
multiplication and division scale elapsed nanoseconds; there is no implicit
floating scalar overload and no plain-number addition, because those call
forms would leave the unit ambiguous.

`+=` and `-=` are the only mutating operators. They replace the receiver and
return `Duration&`; all other arithmetic operators allocate a result. Signed
nanosecond overflow throws rather than wrapping. Division by zero also throws.
`NaT` propagates through arithmetic.

Every comparison is elementwise and returns a shape-preserving Boolean
`Series`, not one host `bool`. This permits broadcasting and masking. `NaT`
does not compare as an elapsed value: use `isnat()` when absence itself is the
question.

Stream insertion uses the same renderer as `pluss::duration::print()`. It
preserves shape and prints negative spans with normalized day and clock fields.

## Examples

Example 1. Binary, unary, and scalar arithmetic.

```cpp
auto hour = pluss::duration::hours(1);
auto ninety = pluss::duration::minutes(90);

std::cout << (hour + ninety) << "\n";
std::cout << -hour << "\n";
std::cout << hour * 3 << "\n";
std::cout << pluss::duration::days(1) / 4;
```

These produce `2:30`, `-1:00`, `3:00`, and `6:00` elapsed durations,
respectively. Division truncates at integral nanosecond precision.

Example 2. Compound assignment mutates and remains equivalent to its binary
counterpart.

```cpp
auto value = hour;
value += ninety;
std::cout << pluss::series::print(value.eq(hour + ninety));

value -= ninety;
std::cout << pluss::series::print(value.eq(hour));
```

Both checks print scalar `true` Boolean Series. Broadcasting rules are the
same as for `+` and `-`; mutation does not weaken shape validation.

Example 3. Comparisons build masks rather than collapsing an array.

```cpp
auto left = pluss::duration::minutes({30, 60, 90});
auto right = pluss::duration::hours(1);
std::cout << pluss::series::print(left < right) << "\n";
std::cout << pluss::series::print(left >= right);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, false]
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, true, true]
```

Example 4. Direct streaming and the named printer are identical.

```cpp
auto value = pluss::duration::minutes(90);
std::ostringstream out;
out << value;
std::cout << std::boolalpha
          << (out.str() == pluss::duration::print(value));
```

### Output

```text
true
```

Example 5. Negative sub-day output is floor-normalized.

```cpp
std::cout << pluss::duration::nanos(-1);
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
-1 days 23:59:59.999999999
```

Example 6. Overflow is a checked failure.

```cpp
auto high = pluss::duration::nanos(
    (std::numeric_limits<std::int64_t>::max)());
auto one = pluss::duration::nanos(1);
auto invalid = high + one;
```

### Output

```text
Duration::operator+: nanosecond overflow
```

## Neighboring Named Methods

The dedicated [`add`](add.md), [`sub`](sub.md), [`neg`](neg.md),
[`mul`](mul.md), and [`div`](div.md) pages teach each arithmetic contract in
isolation. [`eq`](eq.md) through [`ge`](ge.md) do the same for comparisons.
`floordiv()` and `mod()` are named methods because C++ has no distinct operator
for floor division and the public remainder takes a nanosecond integer.

