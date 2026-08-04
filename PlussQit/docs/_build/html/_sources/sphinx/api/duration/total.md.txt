<!-- pluss-hand-authored -->

# Duration::total_seconds

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::total_nanoseconds() const;
Series Duration::total_microseconds() const;
Series Duration::total_milliseconds() const;
Series Duration::total_seconds() const;
Series Duration::total_minutes() const;
Series Duration::total_hours() const;
Series Duration::total_days() const;
Series Duration::total_weeks() const;
```

## What It Does

Converts a `Duration`'s elapsed magnitude into a plain `Series` expressed in
one chosen unit — the way out of duration-space back into an ordinary numeric
`Series`, e.g. for storing elapsed time in a DataFrame column (recall
`Duration` itself isn't a `ThreadKind` payload — see the
[API reference overview](../../duration_api_reference)) or feeding it into
arithmetic that doesn't care about the duration/instant distinction.

Every member here is the same underlying value, `total_nanoseconds()`, just
divided by a different constant — `total_seconds()` divides by `1e9`,
`total_days()` by `86'400 * 1e9`, and so on. None of them round; a `Duration`
of `90` seconds read through `total_minutes()` gives `1.5`, not `1` or `2`.

## Parameters

None — these are all zero-argument accessors on `self`.

## Returns

`Series`
    A floating-point `Series` at `self`'s shape, in the named unit.

## Examples

Example 1. The same `90`-minute `Duration` read through three different
units.

```cpp
auto ninety_min = pluss::duration::minutes(90);
std::cout << "seconds=" << pluss::print_series(ninety_min.total_seconds()) << "\n";
std::cout << "minutes=" << pluss::print_series(ninety_min.total_minutes()) << "\n";
std::cout << "hours=" << pluss::print_series(ninety_min.total_hours());
```

### Output

```text
seconds=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5400.0000
minutes=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=90.0000
hours=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1.5000
```

Same `90`-minute value throughout; only the unit — and therefore the
magnitude — changes. `total_hours()` gives the non-integer `1.5`, confirming
no unit here rounds.

Example 2. `total_days()` over an array `Duration`.

```cpp
auto ranged = pluss::duration::range(
    pluss::duration::days(0), pluss::duration::days(10), pluss::duration::days(2));
std::cout << pluss::print_series(ranged.total_days());
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0.0000, 2.0000, 4.0000, 6.0000, 8.0000, 10.0000]
```

The array shape carries through the conversion — one converted value per
element, same as every other unit accessor here.

## Notes

- `total_*` never rounds or truncates — the result is always the exact
  (floating-point) division, which is why `total_minutes()` on a `90`-second
  `Duration` correctly reads `1.5`, not `1`.
- There are convenience aliases beyond the eight listed in the signature
  (`totalSeconds()`, `to_seconds()`, `total_sec()`, and similar camelCase/
  short-name spellings) — all route to the same underlying `total(unit)`
  dispatch and are not separately documented here.
- To go the other direction (a number back into a `Duration`), use one of the
  [construction](construction.md) factories, not a `total_*` method.

## See Also

- [`construction`](construction.md): the inverse direction — building a
  `Duration` from a plain count in a chosen unit.
- [`abs`](abs.md): magnitude and sign, useful before converting a `Duration`
  that might be negative.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="abs.html">
    <span class="label">Previous</span>
    <span class="title">abs</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
