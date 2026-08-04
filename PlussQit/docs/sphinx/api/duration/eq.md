<!-- pluss-hand-authored -->

# Duration::eq

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::eq(const Duration& other) const;
Series Duration::ne(const Duration& other) const;
Series Duration::lt(const Duration& other) const;
Series Duration::le(const Duration& other) const;
Series Duration::gt(const Duration& other) const;
Series Duration::ge(const Duration& other) const;

Series Duration::operator==(const Duration& other) const;  // eq(other)
Series Duration::operator!=(const Duration& other) const;  // ne(other)
Series Duration::operator<(const Duration& other) const;   // lt(other)
Series Duration::operator<=(const Duration& other) const;  // le(other)
Series Duration::operator>(const Duration& other) const;   // gt(other)
Series Duration::operator>=(const Duration& other) const;  // ge(other)
```

## What It Does

Compares two `Duration` values elementwise by their underlying elapsed-nanosecond
magnitude, broadcasting under ordinary N-D rules, and returns a `Series` of
booleans — one comparison result per broadcasted position.

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

Example 1. `90` minutes is less than `1` hour is false, but `1` hour `<`
`90` minutes is true.

```cpp
auto ninety_min = pluss::duration::minutes(90);
auto one_hour = pluss::duration::hours(1);
std::cout << "eq=" << pluss::print_series(one_hour.eq(ninety_min)) << "\n";
std::cout << "lt=" << pluss::print_series(one_hour.lt(ninety_min)) << "\n";
```

### Output

```text
eq=Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=false
lt=Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

`1 hour == 90 minutes` is `false` (`60 min != 90 min`); `1 hour < 90 minutes`
is `true` (`60 < 90`).

Example 2. `>=` against a smaller duration.

```cpp
auto one_day = pluss::duration::days(1);
auto one_hour = pluss::duration::hours(1);
std::cout << pluss::print_series(one_day.ge(one_hour));
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

`1 day >= 1 hour` is `true`.

## Notes

- Comparisons are purely by elapsed magnitude — there is no notion of
  "direction" beyond sign; a negative `Duration` (see
  [`abs`](abs.md)/[`sign`](abs.md)) compares as less than a positive one, same
  as ordinary signed-number comparison.
- Every operator here is documented sugar for the underlying named method
  (`operator==` is exactly `eq`, etc.) — nothing is computed differently
  between the spellings.

## See Also

- [`arithmetic`](arithmetic.md): combining durations instead of comparing them.
- [`abs`](abs.md): magnitude and sign, relevant when comparing durations that
  may be negative.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="arithmetic.html">
    <span class="label">Previous</span>
    <span class="title">arithmetic</span>
  </a>
  <a class="next-link" href="abs.html">
    <span class="label">Next</span>
    <span class="title">abs</span>
  </a>
</div>
