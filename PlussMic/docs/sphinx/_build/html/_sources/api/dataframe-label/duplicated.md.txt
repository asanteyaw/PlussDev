<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 051f8ce6a6b0 -->

# DataFrameLabelAccessor::duplicated

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::duplicated(bool keep_first = true, bool keep_last = false) const -> Slate
DataFrameLabelAccessor::duplicated(pluss::msharp::RangeView::Of<std::string_view> names, bool keep_first = true, bool keep_last = false) const -> Slate
DataFrameLabelAccessor::duplicated(std::initializer_list<std::string_view> names, bool keep_first = true, bool keep_last = false) const -> Slate
```

## What It Does

Marks each row `true` if its case-label key has already appeared earlier (or,
with `keep_last = true`, will appear later) among the rows sharing that key —
`false` for the first (or, with `keep_last`, the last) occurrence of every
distinct key, and `false` for any key that never repeats at all. With `names`
given, the key is built from just those levels of a structured frame (still
requires `case_axis_` to be defined); without `names`, the key is the full
flat case-label value.

## Parameters

**names** (name-only overloads)

Level names to key the duplicate check on, instead of the full case label.
Requires a structured frame.

**keep_first** (`bool`, default `true`)

If `true`, the first occurrence of each repeated key is `false` (not marked
duplicate); every later occurrence is `true`.

**keep_last** (`bool`, default `false`)

If `true`, the *last* occurrence is spared instead; every earlier one is
`true`. Cannot be combined with `keep_first = true`.

## Returns

`Slate`
    A boolean slate, one entry per row, `display_name` `"duplicated"`,
    displayed against the case label(s) it was computed from.

## Examples

Example 1. Mark duplicates on the full case label, with one repeated value.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region");

auto out = labeled.label().duplicated();
std::cout << out;
```

### Output

```text
 east  false
 west  false
 east  true
north  false
Name: duplicated, ptype: Series
```

`"east"` appears at rows `0` and `2`; with the default `keep_first = true`,
row `0` is spared (`false`) and row `2` is marked (`true`). `"west"` and
`"north"` never repeat, so both are `false`.

Example 2. Key the check on just one level of a structured, multi-level frame.

```cpp
auto readings = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Slate(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});
auto grouped = readings.label().set({"region", "city"}, true, "/");

auto out2 = grouped.label().duplicated({"region"});
std::cout << out2;
```

### Output

```text
east/Boston  false
   east/NYC  true
    west/LA  false
    west/SF  true
Name: duplicated, ptype: Series
```

The duplicate check keys only on `region` (`"east"`, `"east"`, `"west"`,
`"west"`), so the second row of each region group is marked — but the result
is still displayed against the **full** composite case label (`"east/Boston"`,
not just `"east"`), not the restricted key it was computed from.

Example 3. Spare the *last* occurrence instead of the first, with
`keep_last = true`.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region");

auto out = labeled.label().duplicated(false, true);
std::cout << out;
```

### Output

```text
 east  true
 west  false
 east  false
north  false
Name: duplicated, ptype: Series
```

Compare with Example 1's default (`keep_first = true`): there, row `0`
(`"east"`'s first occurrence) was spared and row `2` was marked. Here, with
`keep_first` explicitly set to `false` and `keep_last = true`, it's the
opposite — row `2` (`"east"`'s *last* occurrence) is spared and row `0` is
marked instead.

Example 4. Pass `keep_first = true` together with `keep_last = true`, to see
the call throw.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region");

try {
  auto out = labeled.label().duplicated(true, true);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::duplicated: keep_first and keep_last cannot both be true
```

Sparing both the first *and* the last occurrence isn't a coherent request, so
it throws rather than picking one silently. Note that `keep_first` defaults
to `true` — passing only `keep_last = true` positionally (without also
setting `keep_first = false`, as Example 3 does) hits this same throw.

## Notes

- `keep_first` and `keep_last` cannot both be `true`.
- The displayed labels alongside the result are always the frame's full case
  labels, even when `names` restricts which levels build the duplicate key.
- Unlike [`repeated()`](repeated.md), a key that occurs more than once still
  has exactly one `false` row (its spared occurrence) here.

## See Also

- [`DataFrameLabelAccessor::repeated`](repeated.md): marks **every** occurrence
  of a repeated key `true`, with no spared occurrence.
- [`DataFrameLabelAccessor::drop`](drop.md): row-filter mode built directly on
  this same duplicate/repeated marking.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
