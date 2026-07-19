<!-- pluss-hand-authored -->

# DataFrame::ndistinct

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::ndistinct(pluss::RangeView<std::string_view> features = {}, bool dropnull = true) const -> Thread
```

## What It Does

Counts each selected feature's own distinct-value count independently, returning one
`Int64` entry per feature, labeled by feature name. This is a per-feature count, not a
cross-feature distinct-*combination* count — `ndistinct({"city", "product"})` reports
how many distinct cities and how many distinct products there are, each on its own,
never how many distinct `(city, product)` pairs exist. For that, see
[`freq()`](freq.md) or [`duplicated()`](duplicated.md), which do build a composite key
across several features together.

## Parameters

**features** (`pluss::RangeView<std::string_view>`, default `{}`)

Which features to count. Leave empty (the default) to count every feature on the
frame.

**dropnull** (`bool`, default `true`)

Whether a missing value counts as its own distinct "value" (`false`) or is excluded
from the distinct count entirely (`true`, the default).

## Returns

`Thread`
    One `Int64` entry per selected feature, labeled by feature name, named
    `"ndistinct"`.

## Examples

Example 1. Count distinct values per feature on a small frame.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",   Thread(pluss::make_series({1, 2, 3, 4}))},
    {"city", Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi"}))},
});
std::cout << orders;
```

### Output

```text
===================
     | id    city  
-------------------
0    | 1     Accra 
1    | 2     Tema  
2    | 3     Accra 
3    | 4     Kumasi
-------------------
[4 cs x 2 feat]
```

```cpp
auto counts = orders.ndistinct();
std::cout << counts;
```

### Output

```text
  id  4
city  3
Name: ndistinct, ptype: Series
```

`id` has `4` distinct values (every row is unique); `city` has `3` distinct values
(`Accra`, `Tema`, `Kumasi` — `Accra` repeats, but that doesn't add a fourth distinct
value). These are two independent, per-feature counts — not a count of distinct
`(id, city)` combinations.

## Notes

- This is a per-feature count computed independently for each selected feature; it
  never builds a composite cross-feature key the way [`freq()`](freq.md) and
  [`duplicated()`](duplicated.md) do.
- With `dropnull = false`, a missing value counts as one additional distinct "value"
  for that feature, rather than being excluded from the count.
- Every selected feature name must exist on the frame; an unknown name throws.

## See Also

- [`DataFrame::freq`](freq.md): counts occurrences per distinct value (or
  combination), rather than just the number of distinct values.
- [`DataFrame::duplicated`](duplicated.md): flags individual duplicate rows across a
  composite key, complementary to this method's per-feature count.
- [`DataFrame::nfeatures`](nfeatures.md), [`DataFrame::ncases`](ncases.md): the plain
  shape counts this method's output is labeled against.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ncases.html">
    <span class="label">Previous</span>
    <span class="title">ncases</span>
  </a>
  <a class="next-link" href="ne.html">
    <span class="label">Next</span>
    <span class="title">ne</span>
  </a>
</div>
