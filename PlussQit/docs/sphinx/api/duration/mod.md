<!-- pluss-hand-authored -->

# Duration::mod

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::mod(std::int64_t k) const;
```

## What It Does

Computes the remainder of a `Duration`'s underlying nanosecond count after
division by `k` — unlike [`mul`](mul.md)/[`div`](div.md)/[`floordiv`](floordiv.md),
whose `k` is a plain unit-agnostic count, `mod`'s `k` is always a raw
nanosecond divisor. Computing "remainder after dividing by 3 days" means
passing `3 * 86'400 * 1'000'000'000LL`, not `3` — passing the bare integer `3`
would compute "nanoseconds mod 3 nanoseconds," not "mod 3 days."

## Parameters

**k** (`std::int64_t`)

A raw nanosecond divisor — not a plain count the way `mul`/`div`/`floordiv`'s
`k` is. Convert explicitly (see Example 1).

## Returns

`Duration`
    A new duration holding the elementwise remainder.

## Examples

Example 1. `mod` against a three-day divisor, over a `Duration` array.

```cpp
auto ranged = pluss::duration::range(
    pluss::duration::days(0), pluss::duration::days(10), pluss::duration::days(2));
constexpr std::int64_t kThreeDaysNanos = 3LL * 86'400 * 1'000'000'000LL;
std::cout << pluss::duration::print(ranged.mod(kThreeDaysNanos));
```

### Output

```text
Duration(shape=[6], dim=1, numel=6)
[ 0 days 00:00:00.000000000, 2 days 00:00:00.000000000, 1 days 00:00:00.000000000, 0 days 00:00:00.000000000, 2 days 00:00:00.000000000, 1 days 00:00:00.000000000 ]
```

`[0, 2, 4, 6, 8, 10]` days each mod `3` days, elementwise: `0, 2, 1, 0, 2, 1`.
`k` had to be `3 * 86'400 * 1'000'000'000` (three days' worth of nanoseconds),
not the bare integer `3` — passing `3` would compute "days mod 3
nanoseconds," not "days mod 3 days."

## Notes

- `mod`'s divisor is always raw nanoseconds, regardless of what unit you're
  conceptually thinking in — this is the one arithmetic method here that
  doesn't take a plain, unit-agnostic count.
- There is no `Duration.mod(Duration)` overload — see [`mul`](mul.md)'s notes
  for why elapsed-time-times-elapsed-time isn't representable here.

## See Also

- [`floordiv`](floordiv.md): the quotient complementary to this remainder.
- [`div`](div.md): exact division by a plain count.
