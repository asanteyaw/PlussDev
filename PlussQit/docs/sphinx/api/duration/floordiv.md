<!-- pluss-hand-authored -->

# Duration::floordiv

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::floordiv(std::int64_t k) const;
```

## What It Does

Divides a `Duration` by a plain integer count `k`, flooring the underlying
nanosecond count to the nearest whole nanosecond — the integer-division
counterpart to [`div`](div.md), which divides exactly. `k` divides the
underlying nanosecond count directly, unit-agnostic like `mul`/`div`.

## Parameters

**k** (`std::int64_t`)

A plain integer divisor, applied to the whole `Duration` (every element, if
`self` is N-D).

## Returns

`Duration`
    A new duration holding the elementwise, nanosecond-floored result.

## Examples

Example 1. Floor-divide an array `Duration` by `3`.

```cpp
auto ranged = pluss::duration::range(
    pluss::duration::days(0), pluss::duration::days(10), pluss::duration::days(2));
std::cout << pluss::duration::print(ranged.floordiv(3));
```

### Output

```text
Duration(shape=[6], dim=1, numel=6)
[ 0 days 00:00:00.000000000, 0 days 16:00:00.000000000, 1 days 08:00:00.000000000, 2 days 00:00:00.000000000, 2 days 16:00:00.000000000, 3 days 08:00:00.000000000 ]
```

`[0, 2, 4, 6, 8, 10]` days each floor-divided by `3`: `2 days / 3` lands on
`0 days 16:00:00` (not a whole number of days), `4 days / 3` on
`1 days 08:00:00`, and so on — the flooring happens at nanosecond
granularity, not at the whole-day level, which is exactly why the result
isn't itself a whole number of days.

## Notes

- `floordiv` floors at the nanosecond level, not at the level of whatever
  unit you read the result back through — a `.total().days()` reading can
  still show a non-integer number of days even though the underlying
  nanosecond count was floored exactly.
- There is no `Duration.floordiv(Duration)` overload — see [`mul`](mul.md)'s
  notes for why elapsed-time-times-elapsed-time isn't representable here.

## See Also

- [`div`](div.md): the exact (non-flooring) counterpart.
- [`mod`](mod.md): the remainder complementary to this floor-division.
- [`mul`](mul.md): scaling in the other direction.
