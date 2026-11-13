<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6e502bc39d48 -->

# DataFrame::ndistinct

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
Slate DataFrame::ndistinct(bool dropnull = true) const
```

## What It Does

Counts each feature's own distinct-value count independently, returning one `Int64`
entry per feature currently on the frame, labeled by feature name. This is a
per-feature count, not a cross-feature distinct-*combination* count —
`orders.features({"city", "product"}).ndistinct()` reports how many distinct cities
and how many distinct products there are, each on its own, never how many distinct
`(city, product)` pairs exist. For that, see [`freq()`](freq.md) or
[`duplicated()`](duplicated.md), which do build a composite key across several
features together.

`ndistinct()` carries no feature selector of its own: it always counts over the
*entire* current frame, so narrow first with [`features()`](features.md) when only
some features should be counted.

## Parameters

**dropnull** (`bool`, default `true`)

Whether a missing value counts as its own distinct "value" (`false`) or is excluded
from the distinct count entirely (`true`, the default).

## Returns

`Slate`
    One `Int64` entry per selected feature, labeled by feature name, named
    `"ndistinct"`.

## Examples

Example 1. Count distinct values per feature on a small frame.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",   Slate(pluss::make_series({1, 2, 3, 4}))},
    {"city", Slate(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi"}))},
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

- This is a per-feature count computed independently for each feature on the current
  frame; it never builds a composite cross-feature key the way [`freq()`](freq.md) and
  [`duplicated()`](duplicated.md) do.
- With `dropnull = false`, a missing value counts as one additional distinct "value"
  for that feature, rather than being excluded from the count.
- `ndistinct()` has no feature selector of its own — use [`features()`](features.md)
  to narrow the frame first.

## See Also

- [`DataFrame::freq`](freq.md): counts occurrences per distinct value (or
  combination), rather than just the number of distinct values.
- [`DataFrame::duplicated`](duplicated.md): flags individual duplicate rows across a
  composite key, complementary to this method's per-feature count.
- [`DataFrame::nfeatures`](nfeatures.md), [`DataFrame::ncases`](ncases.md): the plain
  shape counts this method's output is labeled against.
- [`DataFrame::features`](features.md): narrow the frame to the feature(s)
  `ndistinct()` should count.

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
