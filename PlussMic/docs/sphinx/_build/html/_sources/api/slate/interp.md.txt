<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9d8df891dcd9 -->

# Slate::interp

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::interp(std::int64_t limit = -1) const -> Slate
```

## What It Does

Fills missing numeric values by **linear interpolation** between the nearest
valid value before and after each gap — unlike [`ffill()`](ffill.md)/
[`bfill()`](bfill.md), which copy an edge value verbatim, `interp()` computes a
value proportional to each missing position's place within the gap. For a gap of
length `g` between a valid value `left` at position `p` and a valid value
`right` at position `p + g + 1`, the missing entry at `p + k` (for `k` from `1`
to `g`) becomes `left + (right - left) * (k / (g + 1))` — an even linear ramp
between the two anchors.

Only **interior** gaps (bounded by a valid value on both sides) are filled. A
leading run of missing values (nothing valid before it) and a trailing run
(nothing valid after it) are left untouched — there's no second anchor to
interpolate toward. Restricted to numeric `Series` payloads only; every other
kind throws.

## Parameters

**limit** (`std::int64_t`, default `-1`)
    Maximum number of consecutive missing positions to fill per gap, counted from
    the left anchor forward — the same convention as [`ffill()`](ffill.md)'s
    `limit`. `-1` means unlimited (fill the entire gap). Must be `-1` or `>= 0`,
    or it throws.

## Returns

`Slate`
    A new slate with eligible interior gaps interpolated. A slate with no
    missing values, or `limit == 0`, or an empty slate, is returned unchanged.

## Examples

Example 1. A gap of two between `10.0` and `40.0` fills with an even linear ramp:
`20.0`, `30.0`.

```cpp
using namespace pluss::table;

auto readings = Slate(pluss::make_series({
    10.0,
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
    40.0,
    50.0,
}));
auto out = readings.interp();
std::cout << out;
```

### Output

```text
0  10
1  20
2  30
3  40
4  50

ptype: Series
```

Example 2. `limit = 1` fills only the first missing position of the gap (counted
from the left anchor forward) — the rest of the gap stays missing.

```cpp
auto out2 = readings.interp(1);
std::cout << out2;
```

### Output

```text
0  10
1  20
2  NULL
3  40
4  50

ptype: Series
```

Position `1` still gets the same interpolated value (`20.0`, its correct place in
the full three-step gap) — `limit` only decides how far the fill reaches, not the
values used to compute it.

Example 3. A leading gap (nothing valid before it) is left untouched — there's no
left anchor to interpolate from.

```cpp
auto leading = Slate(pluss::make_series({
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
    30.0,
}));
auto out3 = leading.interp();
std::cout << out3;
```

### Output

```text
0  NULL
1  NULL
2  30

ptype: Series
```

Example 4. Restricted to numeric `Series` — a `Note`-kind slate throws.

```cpp
auto words = Slate(pluss::make_note({"a", "b"}));
try {
  auto bad = words.interp();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::interp is only valid for numeric Series payload
```

## Notes

- Interior gaps only — leading and trailing missing runs are never filled,
  matching the "no anchor on one side" limitation of `ffill()`/`bfill()`.
- The interpolated value at a given position depends only on the gap's two
  anchors and that position's place within the full gap — `limit` truncates
  *how far* the fill reaches, not the values computed.
- `limit` must be `-1` or `>= 0`; anything else throws `"Slate::interp: limit
  must be -1 or >= 0"`.
- Numeric `Series` only; any other payload kind throws `"Slate::interp is only
  valid for numeric Series payload"`.

## See Also

- [`Slate::ffill`](ffill.md) / [`Slate::bfill`](bfill.md): fill gaps by copying
  an edge value instead of interpolating.
- [`Slate::fillnull`](fillnull.md): fill every missing entry with one fixed
  value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="info_string.html">
    <span class="label">Previous</span>
    <span class="title">info_string</span>
  </a>
  <a class="next-link" href="invalid.html">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>
