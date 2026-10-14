<!-- pluss-hand-authored -->

# TimeSeriesAccessor::fill_gaps

[Back to TimeSeriesAccessor API Reference](../../slate_ts_accessor_api_reference)

## Signature

```cpp
TimeSeriesAccessor::fill_gaps(GapFillMode mode = GapFillMode::Forward, std::optional<double> constant = std::nullopt) const -> Slate
```

## What It Does

Fills missing entries in the bound slate using one of four strategies:

- **`GapFillMode::Forward`** (the default): each missing position takes the
  nearest **prior** real value.
- **`GapFillMode::Backward`**: each missing position takes the nearest
  **following** real value.
- **`GapFillMode::Zero`**: every missing position becomes `0`.
- **`GapFillMode::Constant`**: every missing position becomes `constant`
  (required for this mode).

A gap that has no real value on the required side at all (a leading gap under
`Forward`, or a trailing gap under `Backward`) is left missing — there's nothing
to fill it with.

## Parameters

**mode** (`GapFillMode`, default `Forward`)
    Which fill strategy to use.

**constant** (`std::optional<double>`, default `std::nullopt`)
    The fill value for `GapFillMode::Constant`. Ignored by every other mode;
    required (throws if absent) when `mode` is `Constant`.

## Returns

`Slate`
    A new slate, same length as the bound source, with gaps filled per `mode`.

## Examples

Example 1. `Forward` (the default) — each gap takes the nearest prior value.

```cpp
using namespace pluss::table;

auto with_gap = Slate(
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out = with_gap.ts().fill_gaps();
std::cout << out;
```

### Output

```text
0  10
1  10
2  30
3  40

ptype: Series
```

Position `1` (masked-missing) takes position `0`'s `10` — the nearest prior real
value. Positions `2` and `3` were never missing, so they're untouched.

Example 2. `Backward` — each gap takes the nearest following value instead.

```cpp
auto out2 = with_gap.ts().fill_gaps(pluss::subsp::stats::ts::GapFillMode::Backward);
std::cout << out2;
```

### Output

```text
0  10
1  30
2  30
3  40

ptype: Series
```

Position `1` now takes position `2`'s `30` — the nearest *following* real
value, the mirror image of Example 1.

Example 3. `Zero` and `Constant` fill every gap with a fixed value, regardless
of neighbors.

```cpp
auto zeroed = with_gap.ts().fill_gaps(pluss::subsp::stats::ts::GapFillMode::Zero);
std::cout << zeroed;

auto constant = with_gap.ts().fill_gaps(pluss::subsp::stats::ts::GapFillMode::Constant, -1.0);
std::cout << constant;
```

### Output

```text
0  10
1  0
2  30
3  40

ptype: Series
0  10
1  -1
2  30
3  40

ptype: Series
```

Example 4. A gap with nothing to fill from stays missing — a leading gap under
`Forward` has no prior value to reach for.

```cpp
auto leading_gap = Slate(
    pluss::make_series({10.0, 20.0, 30.0}),
    pluss::make_series(std::vector<bool>{false, true, true})
);
auto out3 = leading_gap.ts().fill_gaps();
std::cout << out3;
```

### Output

```text
0  NULL
1  20
2  30

ptype: Series
```

Position `0` has no prior position at all, so `Forward` leaves it `NULL` rather
than inventing a value.

Example 5. `Constant` mode without a value throws.

```cpp
try {
  auto bad = with_gap.ts().fill_gaps(pluss::subsp::stats::ts::GapFillMode::Constant);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
subsp::stats::ts::fill_gaps: constant mode requires a value
```

## Notes

- A gap with nothing to fill from on the required side (a leading gap under
  `Forward`, a trailing gap under `Backward`) is left `NULL` rather than
  inventing a value — see Example 4.
- `GapFillMode::Constant` throws
  `"subsp::stats::ts::fill_gaps: constant mode requires a value"` if `constant`
  is absent.
- A pre-existing missing entry in the source slate is what gets filled — this
  method is specifically about closing gaps the mask already marks, not about
  detecting new ones.

## See Also

- [`Slate::ffill`](../slate/ffill.md), [`Slate::bfill`](../slate/bfill.md):
  the equivalent top-level operations outside the `ts()` accessor, with the same
  forward/backward semantics.
- [`TimeSeriesAccessor::align_to`](align_to.md): aligning onto a different time
  axis, optionally filling any newly-introduced gaps the same way.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="expanding.html">
    <span class="label">Previous</span>
    <span class="title">expanding</span>
  </a>
  <a class="next-link" href="ge.html">
    <span class="label">Next</span>
    <span class="title">ge</span>
  </a>
</div>
