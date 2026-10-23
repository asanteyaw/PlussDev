<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6503dd08ea71 -->

# DataFrameLabelAccessor::repeated

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::repeated() const -> Slate
DataFrameLabelAccessor::repeated(pluss::msharp::RangeView::Of<std::string_view> names) const -> Slate
DataFrameLabelAccessor::repeated(std::initializer_list<std::string_view> names) const -> Slate
```

## What It Does

Marks every row `true` whose case-label key occurs more than once anywhere in
the frame — including its *first* occurrence, unlike
[`duplicated()`](duplicated.md), which spares one occurrence (by default, the
first). A key that never repeats is `false` everywhere it appears. With
`names`, the key is built from just those levels of a structured frame
(requires `case_axis_` to be defined); without `names`, the key is the full
flat case-label value.

## Parameters

**names** (name-only overloads)

Level names to key the repeat check on, instead of the full case label.
Requires a structured frame.

## Returns

`Slate`
    A boolean slate, one entry per row, `display_name` `"repeated"`,
    displayed against the case label(s) it was computed from.

## Examples

Example 1. Mark repeats on the full case label — every occurrence, not just
the extras.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.labels().set("region");

auto out = labeled.labels().repeated();
std::cout << out;
```

### Output

```text
 east  true
 west  false
 east  true
north  false
Name: repeated, ptype: Series
```

Both `"east"` rows are `true` — including row `0`, the first occurrence, which
[`duplicated()`](duplicated.md) would mark `false` by default.

Example 2. Key the check on just one level of a structured, multi-level
frame, versus the full composite key.

```cpp
auto readings = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Slate(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});
auto grouped = readings.labels().set({"region", "city"}, true, "/");

auto by_region = grouped.labels().repeated({"region"});
std::cout << by_region;

auto by_full_key = grouped.labels().repeated();
std::cout << by_full_key;
```

### Output (repeated({"region"}))

```text
east/Boston  true
   east/NYC  true
    west/LA  true
    west/SF  true
Name: repeated, ptype: Series
```

### Output (repeated(), full composite key)

```text
east/Boston  false
   east/NYC  false
    west/LA  false
    west/SF  false
Name: repeated, ptype: Series
```

Keyed on `region` alone, every row is `true` (both `"east"` and `"west"` occur
twice); keyed on the full composite label (`"east/Boston"`, and so on), every
row is unique, so every row is `false`.

## Notes

- Unlike [`duplicated()`](duplicated.md), there's no `keep_first`/`keep_last`
  option here — every occurrence of a repeated key is marked, with no
  exception.
- The displayed labels alongside the result are always the frame's full case
  labels, even when `names` restricts which levels build the repeat key.

## See Also

- [`DataFrameLabelAccessor::duplicated`](duplicated.md): spares one occurrence
  of each repeated key (first, by default).
- [`DataFrameLabelAccessor::drop`](drop.md): row-filter mode built directly on
  this same repeat marking.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename.html">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="reset.html">
    <span class="label">Next</span>
    <span class="title">reset</span>
  </a>
</div>
