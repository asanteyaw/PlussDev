<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a3790a4d275c -->

# DataFrameLabelAccessor::set

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrame DataFrameLabelAccessor::set(const Slate& labels) const
DataFrame DataFrameLabelAccessor::set(const pluss::Note& labels) const
DataFrame DataFrameLabelAccessor::set(
    std::string_view feature_name,
    bool drop_feature = true) const
DataFrame DataFrameLabelAccessor::set(
    std::initializer_list<std::string_view> feature_names,
    bool drop_features = true,
    std::string_view separator = "\x1F") const
DataFrame DataFrameLabelAccessor::set(
    pluss::msharp::RangeView::Of<std::string_view> feature_names,
    bool drop_features = true,
    std::string_view separator = "\x1F") const
template <detail::StringViewNameRange NameRange>
DataFrame DataFrameLabelAccessor::set(
    const NameRange& feature_names,
    bool drop_features = true,
    std::string_view separator = "\x1F") const
DataFrame DataFrameLabelAccessor::set(const DataFrame::SetLabelOptions& options) const
```

## What It Does

Two label sources share this name:

- **The direct-label overloads** replace the flat case labels from a `Slate` or `Note`.
  `Slate` is the general form and can carry any supported label payload; `Note` is the
  convenience form for any closed `Note` dtype and is exactly equivalent to first wrapping
  that `Note` in a `Slate`. Neither form creates named levels, so [`has()`](has.md) is
  `false` afterward.
- **The feature-name overloads** (single name, multiple names, or the full
  `SetLabelOptions`) promote one or more existing features to **structured**
  case labels — a real `case_axis_` with one named level per promoted feature.
  With more than one feature, the levels are joined into a composite display
  string using `separator` (default: an unprintable "unit separator"
  character). `drop_feature(s)` controls whether the promoted feature(s) also
  disappear from the ordinary feature columns.

```{important} Promotion removes the source feature by default
Promoting a feature to the case-label axis **removes it from the frame's ordinary
columns by default**. Pass `drop_feature(s) = false` (or
`SetLabelOptions{.drop = false}`) explicitly if the promoted feature should remain
accessible as an ordinary column as well. This matches
[`unstack()`](../dataframe/unstack.md)'s `UnstackOptions::drop = true` default and is the
counterpart to [`reset()`](reset.md), whose job is to materialize labels back into columns.
```

Every overload returns a new frame. The receiver is not mutated, and the feature payloads
are shared or reused according to normal `DataFrame` value semantics; changing labels does
not recalculate the ordinary feature values.

## Parameters

**labels** (`const Slate&` or `const pluss::Note&`)

Replacement flat case labels. Their element count must equal `ncases()`. The `Note`
overload accepts any `Note` dtype; use the general `Slate` overload for `Series`,
`Timeline`, `Duration`, or already-threaded nullable labels. Neither overload creates
structured levels.

**feature_name** / **feature_names** (feature-name overloads)

The existing feature or features to promote to case labels. Multiple names may be supplied
as an initializer list, an explicit `RangeView`, or any input range whose elements convert
to `std::string_view`, such as `std::array<std::string_view, N>` or
`std::vector<std::string>`. The range is consumed during the call; the result owns the
level names and does not borrow them.

**drop_feature** / **drop_features** (`bool`, default `true`)

Whether the promoted feature(s) are removed from the ordinary feature columns
after promotion. Defaults to `true` — see the important callout above.

**separator** (`std::string_view`, default `"\x1F"`, multi-feature overloads)

The string joining each level's value into the composite display label. Only
affects the *displayed* text — [`part(name)`](part.md) always reads the
level's own values regardless of separator.

**options** (`const DataFrame::SetLabelOptions&`)

The full form: `features`, `drop` (default `true`, same as above), `separator`, plus `sort`
(immediately sort rows by the new case labels) and `verify` (throw if the new case labels
aren't unique).

## Returns

`DataFrame`
    A new frame with the requested flat labels or structured case-label levels. Shape and
    row order are unchanged unless `SetLabelOptions.sort` requests ordering.

## Examples

Example 1. Promote a single feature with the default: it becomes the case label and is
removed from the ordinary feature columns.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});

auto out = orders.labels().set("region");
std::cout << out;
```

### Output

```text
=============
      | sales
-------------
east  | 100  
west  | 200  
east  | 150  
north | 90   
-------------
[4 cs x 1 feat]
```

`"region"` becomes the case labels (visible in the leftmost column) and is dropped from the
ordinary feature columns — `out` now has only `sales` as a feature, since `drop_feature`
defaults to `true`.

Example 2. Promote and explicitly keep the feature, so it exists both as the case label
and as an ordinary column.

```cpp
auto out2 = orders.labels().set("region", false);
std::cout << out2;
```

### Output

```text
=====================
      | region  sales
---------------------
east  | east    100  
west  | west    200  
east  | east    150  
north | north   90   
---------------------
[4 cs x 2 feat]
```

Passing `false` explicitly is required to keep `region` as a feature column — this is the
opt-in case, not the default.

Example 3. Promote two features into one composite, structured case label with a custom
separator, keeping the default drop behavior.

```cpp
auto readings = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Slate(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});

auto grouped = readings.labels().set({"region", "city"}, true, "/");
std::cout << grouped;
```

### Output

```text
===================
            | sales
-------------------
east/Boston | 10   
east/NYC    | 20   
west/LA     | 30   
west/SF     | 40   
-------------------
[4 cs x 1 feat]
```

`grouped` now has a genuine 2-level `case_axis_` (`region`, `city`); the composite display
text uses the custom `"/"` separator here, and that separator is preserved by later
operations that rebuild the composite string from the levels, such as
[`order()`](order.md)/[`swap()`](swap.md) and [`cast()`](../dataframe/cast.md). Passing `true` here
explicitly is redundant with the default, but spelled out for clarity alongside the custom
separator argument.

Example 4. The same two-feature promotion, but keeping both `region` and `city` as
ordinary columns too.

```cpp
auto grouped_kept = readings.labels().set({"region", "city"}, false, "/");
std::cout << grouped_kept;
```

### Output

```text
===================================
            | region  city    sales
-----------------------------------
east/Boston | east    Boston  10   
east/NYC    | east    NYC     20   
west/LA     | west    LA      30   
west/SF     | west    SF      40   
-----------------------------------
[4 cs x 3 feat]
```

Example 5. `set(const Slate&)` — a raw axis replacement with no structure at all, and no
`drop` parameter (there is nothing to drop, since it isn't sourced from an existing
column).

```cpp
auto tagged = orders.labels().set(Slate(pluss::make_note({"r1", "r2", "r3", "r4"})));
std::cout << tagged;
```

### Output

```text
====================
     | region  sales
--------------------
r1   | east    100  
r2   | west    200  
r3   | east    150  
r4   | north   90   
--------------------
[4 cs x 2 feat]
```

`tagged.labels().has()` is `false` here — this overload never creates a
`case_axis_`, regardless of the `Slate`'s own kind or origin. Both `region` and `sales`
remain as ordinary columns since this overload doesn't source the labels from a column at
all.

Example 6. Pass an existing `Note` directly; this example uses text labels. The overload is
equivalent to `set(Slate(labels))` and does not turn the `Note` into a frame feature or a
structured level.

```cpp
auto sales = DataFrame({
    {"city",   pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units",  pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price",  pluss::make_series({3.0, 4.0, 2.5, 5.0})},
});

auto labeled = sales.labels().set(
    pluss::make_note({"row-a", "row-b", "row-c", "row-d"}));
std::cout << labeled;
```

### Output

```text
====================================
      | city    region  units  price
------------------------------------
row-a | Accra   south   10     3
row-b | Tema    south   7      4
row-c | Kumasi  middle  12     2.5
row-d | Tamale  north   5      5
------------------------------------
[4 cs x 4 feat]
```

`labeled.labels().has()` remains `false`: these are flat case labels, even though their
storage is a `Note`.

Example 7. Direct labels must match the frame's case count. Validation occurs before a
result is returned.

```cpp
try {
  auto invalid = sales.labels().set(pluss::make_note({"row-a", "row-b"}));
} catch (const std::exception& e) {
  std::string message(e.what());
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
DataFrameLabelAccessor::set: label count must match the frame's case count
```

Example 8. `SetLabelOptions{.sort = true}` — promote (dropping by default, same as
`SetLabelOptions`'s own `drop` default) and immediately sort by the new label.

```cpp
auto sorted = orders.labels().set(DataFrame::SetLabelOptions{.features = {"region"}, .sort = true});
std::cout << sorted;
```

### Output

```text
=============
      | sales
-------------
east  | 100  
east  | 150  
north | 90   
west  | 200  
-------------
[4 cs x 1 feat]
```

Rows are rearranged into ascending order by the new case label (`"east"`, `"east"`,
`"north"`, `"west"`) as part of the same call — equivalent to
`set("region").labels().sort()`. `region` is dropped from the columns since
`SetLabelOptions.drop` defaults to `true`, same as every feature-name overload.

Example 9. `SetLabelOptions{.drop = false, .sort = true}` — keep the column and sort, both
explicit.

```cpp
auto sorted_kept = orders.labels().set(
    DataFrame::SetLabelOptions{.features = {"region"}, .drop = false, .sort = true});
std::cout << sorted_kept;
```

### Output

```text
=====================
      | region  sales
---------------------
east  | east    100  
east  | east    150  
north | north   90   
west  | west    200  
---------------------
[4 cs x 2 feat]
```

Example 10. `SetLabelOptions.verify = true` throws when the resulting case labels aren't
unique.

```cpp
try {
  auto verified = orders.labels().set(DataFrame::SetLabelOptions{.features = {"region"}, .verify = true});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::set_caselbl: duplicate structured case-label key found; set verify=false to allow duplicates
```

`"region"` has `"east"` twice (rows `0` and `2`, same as Example 1's `out`), which
`set("region")` allows freely since `verify` defaults to `false`. Passing `verify = true`
instead rejects the duplicate outright — independent of, and unaffected by, `drop`.

## Notes

- `drop_feature`/`drop_features`/`SetLabelOptions.drop` all default to `true` — promoting a
  feature to the case-label axis removes it from the ordinary columns unless you pass
  `false` explicitly. See the important callout above.
- Only the feature-name overloads create a structured `case_axis_`. The `Slate` and `Note`
  overloads never do and have no `drop` concept because their labels are not sourced from
  an existing feature.
- `separator` only affects the displayed composite string. It's preserved
  across operations that rebuild that string from the levels — including
  [`order()`](order.md), [`swap()`](swap.md), [`drop()`](drop.md), and
  [`cast()`](../dataframe/cast.md) — so a custom separator sticks once set.
- `SetLabelOptions.verify = true` throws if the resulting case labels aren't
  unique; without it, duplicates are allowed freely. `verify` and `drop` are independent.

## See Also

- [`DataFrameLabelAccessor::reset`](reset.md): the (asymmetric) inverse — its own `drop`
  option defaults to `false`, the opposite default, since reset's job is to materialize the
  label back into a column.
- [`DataFrame::unstack`](../dataframe/unstack.md): shares the same `drop = true` default
  convention via its own `UnstackOptions`.
- [`DataFrameLabelAccessor::has`](has.md),
  [`DataFrameLabelAccessor::names`](names.md): inspect whether/how the result
  is structured.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reset.html">
    <span class="label">Previous</span>
    <span class="title">reset</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
