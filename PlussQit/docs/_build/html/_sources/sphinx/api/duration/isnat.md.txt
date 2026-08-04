<!-- pluss-hand-authored -->

# Duration::isnat

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::isnat() const;
Series Duration::isvalid() const;
Series Duration::isfinite() const;   // alias for isvalid()
Series Duration::isnan() const;      // alias for isnat()
Duration Duration::fill_nat(const Duration& value) const;
Duration Duration::where(const Series& mask, const Duration& other) const;
```

## What It Does

`Duration` has the same missing-value convention as [`Timeline`](../timeline/isnat.md):
a sentinel value (`Duration::kNaT`) represents "no duration," and `isnat()`/
`isvalid()` test for it elementwise, returning a boolean `Series`.
`fill_nat(value)` replaces every missing position with `value`; `where(mask,
other)` picks `other` wherever `mask` is true and keeps `self` elsewhere —
the same "true selects other" convention `Series::where` uses.

## Parameters

**value** (`const Duration&`, `fill_nat` only)

The replacement for missing positions. Broadcasts against `self`.

**mask** (`const Series&`, `where` only)

A boolean mask, broadcast against `self`. Where `true`, the corresponding
position of `other` is used; where `false`, `self`'s own value is kept.

**other** (`const Duration&`, `where` only)

The replacement source for masked-`true` positions.

## Returns

`Series` for `isnat`/`isvalid`/`isfinite`/`isnan`; `Duration` for `fill_nat`/`where`.

## Examples

Example 1. A valid duration and an explicitly-missing one.

```cpp
auto one_hour = pluss::duration::hours(1);
auto missing = pluss::duration::nanos(pluss::Duration::kNaT);
std::cout << "valid.isnat=" << pluss::print_series(one_hour.isnat()) << "\n";
std::cout << "missing.isnat=" << pluss::print_series(missing.isnat());
```

### Output

```text
valid.isnat=Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=false
missing.isnat=Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

Example 2. `fill_nat` replaces the missing value with a real `Duration`.

```cpp
auto missing = pluss::duration::nanos(pluss::Duration::kNaT);
auto filled = missing.fill_nat(pluss::duration::hours(2));
std::cout << pluss::print_series(filled.total_hours());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.0000
```

`missing` had no real value; after `fill_nat(pluss::duration::hours(2))`, it
reads back as exactly `2` hours.

## Notes

- `Duration::kNaT` is a `static constexpr std::int64_t` sentinel value (shared
  with `Timeline::kNaT` — both alias `Duration::kNaT` under the hood), not a
  distinct type; build a missing `Duration` with
  `pluss::duration::nanos(Duration::kNaT)`.
- `isfinite()`/`isnan()` are pure aliases for `isvalid()`/`isnat()`
  respectively — provided for readers coming from a floating-point-NaN
  background, not a separate concept.
- `where`'s "`true` selects `other`, `false` keeps `self`" direction is easy
  to get backwards when writing the mask — double-check against a small
  example (as in Example 1's `isnat()` output) before trusting a `where` call
  that isn't obviously symmetric.

## See Also

- [`Timeline::isnat`](../timeline/isnat.md): the same convention on the
  instant side.
- [`total`](total.md): converting a filled-in `Duration` to a plain unit, as
  in Example 2.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="total.html">
    <span class="label">Previous</span>
    <span class="title">total</span>
  </a>
</div>
