<!-- pluss-hand-authored -->

# Duration::div

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::div(std::int64_t k) const;
Duration Duration::operator/(std::int64_t k) const;
```

## What It Does

Divides a `Duration` by a plain integer count `k` — not by another
`Duration`, for the same reason [`mul`](mul.md) doesn't take one (the result
would be an elapsed-time-squared or a unitless ratio, neither of which is a
`Duration`). `k` divides the underlying nanosecond count directly, and the
division is exact (floating-point), not floored — see [`floordiv`](floordiv.md)
for the flooring variant.

## Parameters

**k** (`std::int64_t`)

A plain integer divisor, applied to the whole `Duration` (every element, if
`self` is N-D).

## Returns

`Duration`
    A new duration holding the elementwise divided result.

## Examples

Example 1. Split a one-day duration into quarters.

```cpp
auto one_day = pluss::duration::days(1);
std::cout << pluss::duration::print(one_day / 4);
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
0 days 06:00:00.000000000
```

`1 day / 4 = 6 h`.

## Notes

- `div(k)` and `operator/(k)` are the same operation — `operator/` is
  documented sugar for `div`.
- There is no `Duration / Duration` overload — see [`mul`](mul.md)'s notes for
  why.

## See Also

- [`mul`](mul.md): the inverse scaling operation.
- [`floordiv`](floordiv.md): the flooring counterpart, useful when a whole
  number of sub-intervals matters more than the exact fraction.
- [`mod`](mod.md): remainder after division by a raw nanosecond divisor.
