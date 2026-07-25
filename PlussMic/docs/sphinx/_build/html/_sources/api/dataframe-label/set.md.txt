<!-- pluss-hand-authored -->

# DataFrameLabelAccessor::set

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::set(const Thread& labels) const -> DataFrame
DataFrameLabelAccessor::set(std::string_view feature_name, bool drop_feature = true) const -> DataFrame
DataFrameLabelAccessor::set(std::initializer_list<std::string_view> feature_names, bool drop_features = true, std::string_view separator = "\x1F") const -> DataFrame
DataFrameLabelAccessor::set(pluss::msharp::RangeView::Of<std::string_view> feature_names, bool drop_features = true, std::string_view separator = "\x1F") const -> DataFrame
DataFrameLabelAccessor::set(const DataFrame::SetLabelOptions& options) const -> DataFrame
```

## What It Does

Two entirely different mechanisms share this name:

- **`set(const Thread&)`** replaces the case labels directly with an arbitrary
  `Thread` — no structure is created; [`has()`](has.md) is `false` afterward.
- **The feature-name overloads** (single name, multiple names, or the full
  `SetLabelOptions`) promote one or more existing features to **structured**
  case labels — a real `case_axis_` with one named level per promoted feature.
  With more than one feature, the levels are joined into a composite display
  string using `separator` (default: an unprintable "unit separator"
  character). `drop_feature(s)` controls whether the promoted feature(s) also
  disappear from the ordinary feature columns.

```{important} drop defaults to true — pandas set_index() semantics, not "keep everything"
Promoting a feature to the case-label axis **removes it from the frame's ordinary
columns by default**, exactly like pandas' `set_index(keys, drop=True)`. Pass
`drop_feature(s) = false` (or `SetLabelOptions{.drop = false}`) explicitly if you want the
promoted feature to remain accessible as a regular column *in addition to* becoming the
case label. This mirrors [`unstack()`](../dataframe/unstack.md)'s `UnstackOptions::drop`,
which also defaults to `true`; it is the opposite of [`reset()`](reset.md)'s
`ResetLabelOptions::drop`, which defaults to `false` (reset's job is to *materialize* the
label back into a column, so keeping it there by default is the natural counterpart).
```

## Parameters

**labels** (`const Thread&`, one overload)

Replacement case labels, used as-is. Does not create structured levels.

**feature_name** / **feature_names** (feature-name overloads)

The existing feature(s) to promote to case labels.

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
    A new frame with the requested case labels.

## Examples

Example 1. Promote a single feature with the default: it becomes the case label and is
removed from the ordinary feature columns.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Thread(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});

auto out = orders.label().set("region");
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
auto out2 = orders.label().set("region", false);
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
    {{"region", Thread(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Thread(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});

auto grouped = readings.label().set({"region", "city"}, true, "/");
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
[`order()`](order.md)/[`swap()`](swap.md) and [`cast()`](cast.md). Passing `true` here
explicitly is redundant with the default, but spelled out for clarity alongside the custom
separator argument.

Example 4. The same two-feature promotion, but keeping both `region` and `city` as
ordinary columns too.

```cpp
auto grouped_kept = readings.label().set({"region", "city"}, false, "/");
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

Example 5. `set(const Thread&)` — a raw axis replacement with no structure at all, and no
`drop` parameter (there is nothing to drop, since it isn't sourced from an existing
column).

```cpp
auto tagged = orders.label().set(Thread(pluss::make_note({"r1", "r2", "r3", "r4"})));
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

`tagged.label().has()` is `false` here — this overload never creates a
`case_axis_`, regardless of the `Thread`'s own kind or origin. Both `region` and `sales`
remain as ordinary columns since this overload doesn't source the labels from a column at
all.

Example 6. `SetLabelOptions{.sort = true}` — promote (dropping by default, same as
`SetLabelOptions`'s own `drop` default) and immediately sort by the new label.

```cpp
auto sorted = orders.label().set(DataFrame::SetLabelOptions{.features = {"region"}, .sort = true});
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
`set("region").label().sort()`. `region` is dropped from the columns since
`SetLabelOptions.drop` defaults to `true`, same as every feature-name overload.

Example 7. `SetLabelOptions{.drop = false, .sort = true}` — keep the column and sort, both
explicit.

```cpp
auto sorted_kept = orders.label().set(
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

Example 8. `SetLabelOptions.verify = true` throws when the resulting case labels aren't
unique.

```cpp
try {
  auto verified = orders.label().set(DataFrame::SetLabelOptions{.features = {"region"}, .verify = true});
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
- Only the feature-name overloads create a structured `case_axis_`;
  `set(const Thread&)` never does, no matter what, and has no `drop` concept since it isn't
  sourced from an existing column.
- `separator` only affects the displayed composite string. It's preserved
  across operations that rebuild that string from the levels — including
  [`order()`](order.md), [`swap()`](swap.md), [`drop()`](drop.md), and
  [`cast()`](cast.md) — so a custom separator sticks once set.
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
