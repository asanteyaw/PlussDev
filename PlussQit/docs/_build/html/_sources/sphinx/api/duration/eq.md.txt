<!-- pluss-hand-authored -->

# Duration::eq

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::eq(const Duration& other) const;
Series Duration::operator==(const Duration& other) const;  // eq(other)
```

## What It Does

Compares two `Duration` values elementwise by their underlying
elapsed-nanosecond magnitude for equality, broadcasting under ordinary N-D
rules, and returns a `Series` of booleans — one comparison result per
broadcasted position.

```{note} Series[bool], not a scalar bool
This is the same design `Timeline`'s `eq`/`ne`/`lt`/`le`/`gt`/`ge` use, and the
opposite of `Timeline`'s *own* `operator==`/`operator<`/etc. (which return a
plain scalar `bool`, documented on `TimelineBase`). `Duration`'s operators are
all wired to the `Series`-returning methods, with no scalar-`bool` sibling.
```

## Parameters

**other** (`const Duration&`)

The duration to compare against. Broadcasts against `self`.

## Returns

`Series`
    A boolean `Series` at the broadcasted shape, one comparison result per
    position.

## Examples

Example 1. `90` minutes equals `1` hour is false.

```cpp
auto ninety_min = pluss::duration::minutes(90);
auto one_hour = pluss::duration::hours(1);
std::cout << pluss::series::print(one_hour.eq(ninety_min));
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=false
```

`1 hour == 90 minutes` is `false` (`60 min != 90 min`).

## Notes

- Comparisons are purely by elapsed magnitude — there is no notion of
  "direction" beyond sign; a negative `Duration` (see
  [`abs`](abs.md)/[`sign`](sign.md)) compares as less than a positive one,
  same as ordinary signed-number comparison.
- `operator==` is documented sugar for `eq` — nothing is computed differently
  between the two spellings.

## See Also

- [`ne`](ne.md): the negation of this comparison.
- [`lt`](lt.md), [`le`](le.md), [`gt`](gt.md), [`ge`](ge.md): the ordering
  comparisons.
- [`abs`](abs.md): magnitude and sign, relevant when comparing durations that
  may be negative.
