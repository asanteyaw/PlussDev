<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: de32d3eeff2e -->

# DataFrameLabelAccessor::order

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::order(pluss::msharp::RangeView::Of<std::string_view> names) const -> DataFrame
DataFrameLabelAccessor::order(std::initializer_list<std::string_view> names) const -> DataFrame
```

## What It Does

Reorders the case-label **levels** (not rows) to match `names`, which must
name every existing level exactly once. Values within each level are
untouched — only which level comes first, second, and so on in the composite
label changes.

The composite display string is rebuilt from the new level order using
whatever separator (custom or default) was used when the levels were
originally created via [`set()`](set.md) — the separator is preserved across
reordering.

## Parameters

**names** (`pluss::msharp::RangeView::Of<std::string_view>` /
`std::initializer_list<std::string_view>`)

Every current level name, in the new order you want them combined in.

## Returns

`DataFrame`
    A new frame with the same rows and values, its case-label levels reordered.

## Examples

Example 1. Swap the order of a 2-level structured frame's levels.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Slate(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});
auto grouped = readings.labels().set({"region", "city"}, true, "/");
std::cout << grouped;

auto out = grouped.labels().order({"city", "region"});
std::cout << out;
```

### Output (before)

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

### Output (after)

```text
===================
            | sales
-------------------
Boston/east | 10   
NYC/east    | 20   
LA/west     | 30   
SF/west     | 40   
-------------------
[4 cs x 1 feat]
```

`city` now comes first in every composite label (`"Boston"` before `"east"`,
and so on). `grouped` was built with an explicit `"/"` separator, and
`order()` preserves it — the composite label is still `"/"`-joined, just with
the two parts swapped.

Example 2. Pass a partial list — fewer names than existing levels — to see
the call throw.

```cpp
try {
  auto out = grouped.labels().order({"city"});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::order_caselbls: expected every case level exactly once
```

`grouped` has 2 levels (`"region"`, `"city"`), so `order()` requires exactly 2
names; passing only `"city"` throws instead of leaving `"region"` wherever it
was.

Example 3. Pass a duplicated name — repeating one level instead of naming the
other — to see a different throw.

```cpp
try {
  auto out = grouped.labels().order({"city", "city"});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::order_caselbls: duplicate case label 'city'
```

Here the count matches (2 names for 2 levels), but `"city"` is listed twice
and `"region"` not at all — the duplicate is caught explicitly rather than
silently dropping `"region"` from the result.

## Notes

- This reorders **levels**, not rows — row order and every feature column are
  untouched.
- A custom separator passed to [`set()`](set.md) survives `order()` (and
  [`swap()`](swap.md), which is built on it): the displayed composite label
  keeps using whichever separator the levels were created with.
- `names` must include every current level exactly once; a partial or
  duplicated list throws.

## See Also

- [`DataFrameLabelAccessor::swap`](swap.md): swap exactly two levels — a
  convenience built directly on this method.
- [`DataFrameLabelAccessor::names`](names.md): the current level order this
  method reorders.
- [`DataFrameLabelAccessor::rename`](rename.md): change level *names* without
  touching their order.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="part.html">
    <span class="label">Next</span>
    <span class="title">part</span>
  </a>
</div>
