<!-- pluss-hand-authored -->

# Duration::mul

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::mul(std::int64_t k) const;
Duration Duration::operator*(std::int64_t k) const;
```

## What It Does

Scales a `Duration` by a plain integer count `k` — not by another `Duration`,
since "a duration times a duration" isn't a meaningful elapsed-time value
(compare: `Series * Series` is fine because both sides are unitless numbers;
`Duration * Duration` would need to be an elapsed-time-squared, which nothing
here represents). `k` scales the underlying nanosecond count directly, so it
is unit-agnostic — `d * 3` triples `d` regardless of what unit `d` was
originally built from.

## Parameters

**k** (`std::int64_t`)

A plain integer multiplier, applied to the whole `Duration` (every element,
if `self` is N-D).

## Returns

`Duration`
    A new duration holding the elementwise scaled result.

## Examples

Example 1. Triple a one-hour duration.

```cpp
auto one_hour = pluss::duration::hours(1);
std::cout << pluss::duration::print(one_hour * 3);
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
0 days 03:00:00.000000000
```

`1 h * 3 = 3 h`.

## Notes

- `mul(k)` and `operator*(k)` are the same operation — `operator*` is
  documented sugar for `mul`.
- There is no `Duration * Duration` overload — see What It Does for why.

## See Also

- [`div`](div.md): the inverse scaling operation.
- [`floordiv`](floordiv.md): integer (flooring) division by a plain count.
- [`mod`](mod.md): remainder after division by a raw nanosecond divisor.
