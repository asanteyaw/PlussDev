<!-- pluss-hand-authored -->

# DataFrameTsAccessor::fill_gaps

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::fill_gaps(pluss::subsp::stats::ts::GapFillMode mode = pluss::subsp::stats::ts::GapFillMode::Forward, std::optional<double> constant = std::nullopt) const -> DataFrame
```

## What It Does

Fills missing (`NULL`) values in every bound feature according to `mode`:

- **`Forward`** (the default): each gap is filled with the nearest preceding
  non-null value (a "last observation carried forward" fill).
- **`Backward`**: each gap is filled with the nearest following non-null value.
- **`Zero`**: every gap is filled with `0.0`.
- **`Constant`**: every gap is filled with `constant` (which must be supplied, or
  the call throws).

A gap with no qualifying neighbor in the requested direction (for example, leading
nulls under `Forward`, or trailing nulls under `Backward`) remains `NULL`. This
`GapFillMode` (`pluss::subsp::stats::ts::GapFillMode`) has exactly these four
values — `Forward`, `Backward`, `Zero`, `Constant` — and no `None`/no-op member;
don't confuse it with the unrelated `TsReindexFill` enum used by
[`align_to()`](align_to.md), which does have a `None` value.

## Parameters

**mode** (`GapFillMode`, default `Forward`)

The fill strategy, described above.

**constant** (`std::optional<double>`, default `std::nullopt`)

The fill value for `GapFillMode::Constant`. Required (and only meaningful) when
`mode == Constant`.

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    gaps filled according to `mode`.

## Examples

Example 1. Forward-fill and backward-fill the same gapped feature.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"reading_id", Thread(pluss::make_series({1, 2, 3, 4, 5}))},
    {"temp",       Thread(pluss::make_series({68.0, std::nullopt, std::nullopt, 72.0, std::nullopt}))},
});
std::cout << readings;
```

### Output (before)

```text
===========
     | temp
-----------
0    | 68  
1    | NULL
2    | NULL
3    | 72  
4    | NULL
-----------
[5 cs x 1 feat]
```

```cpp
auto forward = readings.features({"temp"}).ts().fill_gaps();
std::cout << forward;
```

### Output (forward)

```text
===========
     | temp
-----------
0    | 68  
1    | 68  
2    | 68  
3    | 72  
4    | 72  
-----------
[5 cs x 1 feat]
```

Cases `1` and `2` both carry forward the last seen value, `68`; case `4` carries
forward `72`.

```cpp
auto backward = readings.features({"temp"}).ts().fill_gaps(pluss::subsp::stats::ts::GapFillMode::Backward);
std::cout << backward;
```

### Output (backward)

```text
===========
     | temp
-----------
0    | 68  
1    | 72  
2    | 72  
3    | 72  
4    | NULL
-----------
[5 cs x 1 feat]
```

Cases `1` and `2` both take the next seen value, `72`. Case `4`'s trailing gap has
no later value to pull from, so it stays `NULL` even under `Backward`.

Example 2. `Zero` fills every gap unconditionally, including the trailing one
`Backward` couldn't reach.

```cpp
auto zero = readings.features({"temp"}).ts().fill_gaps(pluss::subsp::stats::ts::GapFillMode::Zero);
std::cout << zero;
```

### Output

```text
===========
     | temp
-----------
0    | 68  
1    | 0   
2    | 0   
3    | 72  
4    | 0   
-----------
[5 cs x 1 feat]
```

Unlike `Forward`/`Backward`, `Zero` has no notion of "no qualifying neighbor" —
every `NULL`, including case `4`'s trailing gap, becomes `0`.

Example 3. `Constant` fills every gap with a caller-supplied value.

```cpp
auto constant = readings.features({"temp"}).ts().fill_gaps(
    pluss::subsp::stats::ts::GapFillMode::Constant, -1.0);
std::cout << constant;
```

### Output

```text
===========
     | temp
-----------
0    | 68  
1    | -1  
2    | -1  
3    | 72  
4    | -1  
-----------
[5 cs x 1 feat]
```

Same unconditional fill as `Zero`, just with `-1` (the supplied `constant`)
instead of a fixed `0`.

Example 4. `Constant` without a `constant` value throws.

```cpp
try {
  auto out = readings.features({"temp"}).ts().fill_gaps(pluss::subsp::stats::ts::GapFillMode::Constant);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
subsp::stats::ts::fill_gaps: constant mode requires a value
```

`constant` defaults to `std::nullopt`; `GapFillMode::Constant` needs an actual
fill value, so omitting it throws instead of silently falling back to `0.0`
(that's what `GapFillMode::Zero` is for).

## Notes

- `Forward`/`Backward` gaps with no qualifying neighbor in that direction (leading
  nulls under `Forward`, trailing nulls under `Backward`) remain `NULL` — neither
  mode manufactures a value from nothing.
- `Constant` requires `constant` to be supplied; calling it without one throws.
- `Zero`/`Constant` fill **every** gap unconditionally, regardless of its position,
  unlike `Forward`/`Backward`'s directional dependency.

## See Also

- [`DataFrame::fillnull`](../dataframe/fillnull.md): a general-purpose null-filling
  method outside the `ts()` accessor, with its own value/feature-scoped shapes.
- [`DataFrameTsAccessor::insert_gaps`](insert_gaps.md): introducing missing rows for
  absent calendar periods, a different (structural) notion of "gap" than a null
  value in an existing row.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="expanding.html">
    <span class="label">Previous</span>
    <span class="title">expanding</span>
  </a>
  <a class="next-link" href="grouped.html">
    <span class="label">Next</span>
    <span class="title">grouped</span>
  </a>
</div>
