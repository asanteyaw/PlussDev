<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d0588a6fcf96 -->

# Slate::ffill

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate Slate::ffill(std::int64_t limit = -1) const
```

## What It Does

Forward-fills missing entries by propagating the last observed value ahead of
each gap. Scanning left to right, whenever an entry is missing and the entry
immediately before it is (or has just become) valid, it copies that value
forward — so a run of several consecutive missing entries fills entirely from
the single valid value preceding the run, not independently per position.

```{note} what counts as "missing" here
For a floating-point `Series`, `ffill()` treats a plain `NaN` value as missing
even with no validity mask present — it doesn't require an explicit mask entry,
unlike [`isnull()`](isnull.md)/[`dropnull()`](dropnull.md), which look at the
mask only. This is intentional: fill methods exist to patch numeric gaps, and
`NaN` is the natural in-band hole marker for floating-point data. If a validity
mask is also present, both signals combine — an entry is treated as missing if
either the mask marks it invalid or (for floats) it's `NaN`.
```

`limit` caps how many consecutive missing positions get filled from a single
valid anchor; positions beyond the cap stay missing. A **leading** run of missing
entries (nothing valid appears before it) never gets filled, no matter the limit
— there's no earlier value to borrow. Works across every payload kind (`Series`,
`Note`, `Timeline`, `TimeIndex` — the latter delegates through its underlying
`Timeline`).

## Parameters

**limit** (`std::int64_t`, default `-1`)
    Maximum number of consecutive missing positions to fill per run, counted from
    the valid anchor forward. `-1` means unlimited (fill the entire run). Must be
    `-1` or `>= 0`, or it throws.

## Returns

`Slate`
    A new slate with eligible missing positions filled. If the slate had no
    validity mask and nothing was missing, the original slate is returned
    unchanged. `limit == 0` is also a no-op.

## Examples

Example 1. Forward-fill every eligible missing position.

```cpp
using namespace pluss::table;

auto readings = Slate(pluss::make_series({
    10.0,
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
    40.0,
    50.0,
}));
auto out = readings.ffill();
std::cout << out;
```

### Output

```text
0  10
1  10
2  10
3  40
4  50

ptype: Series
```

Both missing positions borrow from the single valid value at position `0` — the
run fills as a block, not by looking two steps back independently.

Example 2. `limit = 1` caps the fill to one position per run; the rest of the run
stays missing.

```cpp
auto out2 = readings.ffill(1);
std::cout << out2;
```

### Output

```text
0  10
1  10
2  NULL
3  40
4  50

ptype: Series
```

Example 3. A leading missing run is never filled — there's nothing before it to
borrow from.

```cpp
auto leading = Slate(pluss::make_series({
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
    30.0,
}));
auto out3 = leading.ffill();
std::cout << out3;
```

### Output

```text
0  NULL
1  NULL
2  30

ptype: Series
```

## Notes

- `limit` counts positions filled from the anchor forward per run — it resets at
  every naturally valid entry, not per call.
- A leading missing run (nothing valid precedes it) always stays missing,
  regardless of `limit`.
- For floating-point `Series`, `NaN` counts as missing even without an explicit
  validity mask — see the note above.
- `limit` must be `-1` or `>= 0`; anything else throws `"Slate::ffill: limit must
  be -1 or >= 0"`.
- Works on every payload kind, including `TimeIndex` (via its underlying
  `Timeline`).

## See Also

- [`Slate::bfill`](bfill.md): propagate the *next* observed value backward
  instead.
- [`Slate::interp`](interp.md): fill numeric gaps by linear interpolation rather
  than copying an edge value.
- [`Slate::fillnull`](fillnull.md): fill every missing entry with one fixed
  value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extract.html">
    <span class="label">Previous</span>
    <span class="title">extract</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
