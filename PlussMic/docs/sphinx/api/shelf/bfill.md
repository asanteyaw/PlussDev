<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6f72decabec0 -->

# Shelf::bfill

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::bfill(std::int64_t limit = -1) const
```

## What It Does

The mirror image of [`ffill()`](ffill.md): scanning right to left, whenever an
entry is missing and the entry immediately after it is (or has just become)
valid, it copies that value backward — so a run of consecutive missing entries
fills entirely from the single valid value following the run.

`limit` caps how many consecutive missing positions get filled from a single
valid anchor per run; positions beyond the cap stay missing. A **trailing** run
of missing entries (nothing valid appears after it) never gets filled — there's
no later value to borrow. Works across every payload kind (`Series`, `Note`,
`Timeline`, `TimeIndex` — the latter delegates through its underlying
`Timeline`).

See [`ffill()`'s note](ffill.md) on what counts as "missing": only entries marked
missing are filled, and a `NaN` is a value, so it is not (Example 4 shows how to
fill across `NaN` holes).

## Parameters

**limit** (`std::int64_t`, default `-1`)

Maximum number of consecutive missing positions to fill per run, counted from
the valid anchor backward. `-1` means unlimited (fill the entire run). Must be
`-1` or `>= 0`, or it throws.

## Returns

`Shelf`
    A new shelf with eligible missing positions filled. If the shelf had no
    validity mask and nothing was missing, the original shelf is returned
    unchanged. `limit == 0` is also a no-op.

## Examples

Example 1. Backward-fill every eligible missing position.

```cpp
using namespace pluss::table;

auto readings = Shelf(pluss::make_series({
    10.0,
    0.0,
    0.0,
    40.0,
    50.0}), pluss::make_series(std::vector<bool>{true, false, false, true, true}));
auto out = readings.bfill();
std::cout << out;
```

### Output

```text
0  10
1  40
2  40
3  40
4  50

ptype: Series
```

Both missing positions borrow from the single valid value at position `3` — the
run fills as a block, working backward from the anchor.

Example 2. `limit = 1` caps the fill to one position per run, counted from the
anchor backward — the position furthest from the anchor stays missing.

```cpp
auto out2 = readings.bfill(1);
std::cout << out2;
```

### Output

```text
0  10
1  NULL
2  40
3  40
4  50

ptype: Series
```

Example 3. A trailing missing run is never filled — there's nothing after it to
borrow from.

```cpp
auto trailing = Shelf(pluss::make_series({
    30.0,
    0.0,
    0.0}), pluss::make_series(std::vector<bool>{true, false, false}));
auto out3 = trailing.bfill();
std::cout << out3;
```

### Output

```text
0  30
1  NULL
2  NULL

ptype: Series
```

Example 4. `NaN` is not missing: mark it missing first to fill across it.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
auto sensor = Shelf(pluss::make_series({1.0, nan, nan, 4.0, 5.0}));

// A NaN is a value, not a missing entry, so bfill() leaves it where it is.
auto untouched = sensor.bfill();
std::cout << untouched << "\n";

// To fill across NaN holes, mark them missing first (an entry that is already missing stays missing).
auto marked = Shelf(sensor.as_series(), sensor.notnull().land(sensor.notnan()).as_series());
auto filled = marked.bfill();
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
1  4
2  4
3  4
4  5
ptype: Series
```

`bfill()` left both `NaN` cells alone. After marking them missing, the same call fills them from the next observed value, `4`.

## Notes

- `limit` counts positions filled from the anchor backward per run — it resets
  at every naturally valid entry, not per call.
- A trailing missing run (nothing valid follows it) always stays missing,
  regardless of `limit`.
- Only entries marked missing are filled. A `NaN` is a value: it is left as it is
  (mark it missing first if it should be filled, as in Example 4).
- `limit` must be `-1` or `>= 0`; anything else throws `"Shelf::bfill: limit
  must be -1 or >= 0"`.
- Works on every payload kind, including `TimeIndex` (via its underlying
  `Timeline`).

## See Also

- [`Shelf::ffill`](ffill.md): propagate the last observed value forward
  instead.
- [`Shelf::interp`](interp.md): fill numeric gaps by linear interpolation rather
  than copying an edge value.
- [`Shelf::fillnull`](fillnull.md): fill every missing entry with one fixed
  value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="between.html">
    <span class="label">Previous</span>
    <span class="title">between</span>
  </a>
  <a class="next-link" href="bottom.html">
    <span class="label">Next</span>
    <span class="title">bottom</span>
  </a>
</div>
