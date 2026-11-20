<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d0588a6fcf96 -->

# Shelf::ffill

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::ffill(std::int64_t limit = -1) const
```

## What It Does

Forward-fills missing entries by propagating the last observed value ahead of
each gap. Scanning left to right, whenever an entry is missing and the entry
immediately before it is (or has just become) valid, it copies that value
forward — so a run of several consecutive missing entries fills entirely from
the single valid value preceding the run, not independently per position.

```{note} what counts as "missing" here
`ffill()` fills **missing** entries: the ones the validity mask marks missing
(`NULL`), exactly the ones [`isnull()`](isnull.md) and [`dropnull()`](dropnull.md)
see. A `NaN` is a value, not a missing entry, so it is **not** filled, and it can be
the "last observed value" that a following missing entry copies. To forward-fill
across `NaN` holes, mark them missing first (Example 4). See
[Missing, NaN and infinity](../../guide/missing_nan_and_inf.md) for the full
distinction.
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

`Shelf`
    A new shelf with eligible missing positions filled. If the shelf had no
    validity mask and nothing was missing, the original shelf is returned
    unchanged. `limit == 0` is also a no-op.

## Examples

Example 1. Forward-fill every eligible missing position.

```cpp
using namespace pluss::table;

auto readings = Shelf(pluss::make_series({
    10.0,
    0.0,
    0.0,
    40.0,
    50.0}), pluss::make_series(std::vector<bool>{true, false, false, true, true}));
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
auto leading = Shelf(pluss::make_series({
    0.0,
    0.0,
    30.0}), pluss::make_series(std::vector<bool>{false, false, true}));
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

Example 4. `NaN` is not missing: mark it missing first to fill across it.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
auto sensor = Shelf(pluss::make_series({1.0, nan, nan, 4.0, 5.0}));

// A NaN is a value, not a missing entry, so ffill() leaves it where it is.
auto untouched = sensor.ffill();
std::cout << untouched << "\n";

// To fill across NaN holes, mark them missing first (an entry that is already missing stays missing).
auto marked = Shelf(sensor.as_series(), sensor.notnull().land(sensor.notnan()).as_series());
auto filled = marked.ffill();
std::cout << filled << "\n";
```

### Output

```text
0  1
1  NaN
2  NaN
3  4
4  5
ptype: Series

0  1
1  1
2  1
3  4
4  5
ptype: Series
```

`ffill()` left both `NaN` cells alone. After marking them missing (`notnull()` and `notnan()` both have to hold for a cell to stay present), the same call fills them from the last observed value, `1`.

## Notes

- `limit` counts positions filled from the anchor forward per run — it resets at
  every naturally valid entry, not per call.
- A leading missing run (nothing valid precedes it) always stays missing,
  regardless of `limit`.
- Only entries marked missing are filled. A `NaN` is a value and is left as it is
  (mark it missing first if it should be filled, as in Example 4) — see the note above.
- `limit` must be `-1` or `>= 0`; anything else throws `"Shelf::ffill: limit must
  be -1 or >= 0"`.
- Works on every payload kind, including `TimeIndex` (via its underlying
  `Timeline`).

## See Also

- [`Shelf::bfill`](bfill.md): propagate the *next* observed value backward
  instead.
- [`Shelf::interp`](interp.md): fill numeric gaps by linear interpolation rather
  than copying an edge value.
- [`Shelf::fillnull`](fillnull.md): fill every missing entry with one fixed
  value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extract.html">
    <span class="label">Previous</span>
    <span class="title">extract</span>
  </a>
  <a class="next-link" href="fillinf.html">
    <span class="label">Next</span>
    <span class="title">fillinf</span>
  </a>
</div>
