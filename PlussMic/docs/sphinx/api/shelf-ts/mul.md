<!-- pluss-hand-authored -->

# TimeSeriesAccessor::mul

[Back to TimeSeriesAccessor API Reference](../../shelf_ts_accessor_api_reference)

## Signature

```cpp
Shelf TimeSeriesAccessor::mul(const Shelf& other, const Shelf& left_axis, const Shelf& right_axis, TsAlignMode mode = TsAlignMode::Exact) const
```

## What It Does

Multiplies the bound shelf by `other` **after first reindexing `other` onto
the bound shelf's own positions**, using `left_axis` (the bound shelf's time
axis) and `right_axis` (`other`'s own time axis) — exactly the same alignment
[`align_to()`](align_to.md) performs, run automatically before an ordinary
[`Shelf::mul`](../shelf/mul.md). See [`add()`](add.md) for the shared
alignment mechanics this method also uses.

## Parameters

**other** (`const Shelf&`)

The shelf to multiply by, currently living at `right_axis` ticks.

**left_axis** (`const Shelf&`)

The bound shelf's own time axis (`Timeline`/`TimeIndex`), same length as
the bound shelf.

**right_axis** (`const Shelf&`)

`other`'s time axis (`Timeline`/`TimeIndex`), same length as `other`.

**mode** (`TsAlignMode`, default `Exact`)

The alignment strategy — same meaning as [`align_to()`](align_to.md)'s
`mode`: `Exact`, `Backward`, `Forward`, or `Nearest`.

## Returns

`Shelf`
    A new shelf, same length as the bound shelf, holding
    `bound * aligned(other)` at each position.

## Examples

Example 1. Two shelves on different daily axes, multiplied after exact
alignment.

```cpp
using namespace pluss::table;

auto left_axis = Shelf(pluss::make_timeline(
    {"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"}));
auto left = Shelf(pluss::make_series({10.0, 20.0, 30.0}));

auto right_axis = Shelf(pluss::make_timeline(
    {"2024-01-02T00:00:00Z", "2024-01-04T00:00:00Z"}));
auto right = Shelf(pluss::make_series({100.0, 200.0}));

auto out = left.ts().mul(right, left_axis, right_axis);
std::cout << out;
```

### Output

```text
0  NULL
1  2000
2  NULL

ptype: Series
```

Only `2024-01-02` has an exact tick match on both sides (`20 * 100 = 2000`);
the other positions have no matching `right_axis` tick under `Exact`, so
they're `NULL`.

Example 2. The same shelves, aligned with `Backward` — `other`'s most recent
known value carries forward.

```cpp
auto out2 = left.ts().mul(right, left_axis, right_axis, TsAlignMode::Backward);
std::cout << out2;
```

### Output

```text
0  NULL
1  2000
2  3000

ptype: Series
```

`2024-01-03` now picks up `right`'s most recent value at or before it
(`2024-01-02`'s `100`): `30 * 100 = 3000`.

Example 3. A `right_axis` whose length doesn't match `other` throws.

```cpp
auto short_axis = Shelf(pluss::make_timeline({"2024-01-02T00:00:00Z"}));
try {
  auto bad = left.ts().mul(right, left_axis, short_axis);
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
TimeSeriesAccessor::mul: right values and right time axis length mismatch
```

## Notes

- `left_axis` must be the same length as the bound shelf, and `right_axis`
  must be the same length as `other`, or it throws a length-mismatch error
  naming whichever side is wrong.
- The alignment step is identical to [`align_to()`](align_to.md).
- The multiplication itself, once `other` is aligned, is an ordinary
  [`Shelf::mul`](../shelf/mul.md) — a `NULL` on either side produces `NULL`.

## See Also

- [`TimeSeriesAccessor::add`](add.md): the same align-then-combine pattern,
  fully worked through with both alignment modes.
- [`Shelf::mul`](../shelf/mul.md): the plain positional multiplication this
  method performs after alignment.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lt.html">
    <span class="label">Previous</span>
    <span class="title">lt</span>
  </a>
  <a class="next-link" href="ne.html">
    <span class="label">Next</span>
    <span class="title">ne</span>
  </a>
</div>
