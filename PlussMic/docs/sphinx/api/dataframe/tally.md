<!-- pluss-hand-authored -->

# DataFrame::tally

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::tally(std::int64_t top_n = 0, bool dropnull = true, bool render = false) const -> DataFrameView
DataFrame::tally(pluss::RangeView<std::string_view> features, std::int64_t top_n = 0, bool dropnull = true, bool render = false) const -> DataFrameView
DataFrame::tally(std::string_view feature, std::int64_t top_n = 0, bool dropnull = true, bool render = false) const -> DataFrameView
```

## What It Does

Counts how many rows share each distinct combination of values across `features`,
returning a compact frame with one row per distinct combination plus a `"count"`
column — sorted by count descending, ties broken alphabetically by the key values
ascending. This is the `DataFrame`-returning sibling of [`freq()`](freq.md): `freq()`
packs the key columns into one composite `Thread` label, while `tally()` keeps each
key feature as its own separate output column, which reads more naturally when
there's more than one key feature to inspect.

Every key column in the result is rendered as text (`Note`), regardless of the
source feature's original dtype — `tally()` builds its composite key by stringifying
every value first, the same convention [`freq()`](freq.md) and
[`duplicated()`](duplicated.md) use.

## Parameters

**features** (`pluss::RangeView<std::string_view>`, required on that overload)

Which features form the key whose distinct combinations are counted. The
single-feature overload (`tally(feature, ...)`) is shorthand for a one-element list.
The no-`features` overload counts over **every** feature on the frame.

**top_n** (`std::int64_t`, default `0`)

Keep only the `top_n` highest-count rows after sorting. `0` (the default) keeps
every row.

**dropnull** (`bool`, default `true`)

When `true`, a row with a missing value in any key feature is excluded before
counting. When `false`, a missing key value contributes the literal text `"NULL"`
to that row's key instead.

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`, identical to [`head()`](head.md)'s
`render` parameter.

## Returns

`DataFrameView`
    One row per distinct key combination, one column per key feature (always
    rendered as text) plus `"count"`, sorted by count descending (ties broken
    alphabetically by key ascending).

## Examples

Example 1. Tally over two named features.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"city",    Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Accra"}))},
    {"product", Thread(pluss::make_note({"A", "B", "A", "B", "A"}))},
    {"sales",   Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0, 95.0}))},
});
std::cout << orders;
```

### Output (before)

```text
============================
     | city   product  sales
----------------------------
0    | Accra  A        100  
1    | Accra  B        120  
2    | Tema   A        90   
3    | Tema   B        150  
4    | Accra  A        95   
----------------------------
[5 cs x 3 feat]
```

```cpp
auto by_pair = orders.tally({"city", "product"});
std::cout << by_pair;
```

### Output (after)

```text
============================
     | city   product  count
----------------------------
0    | Accra  A        2    
1    | Accra  B        1    
2    | Tema   A        1    
3    | Tema   B        1    
----------------------------
[4 cs x 3 feat]
```

`(Accra, A)` occurs twice (cases `0` and `4`), so it sorts first with `count = 2`.
The three remaining combinations each occur once, tied at `count = 1`, so they're
ordered alphabetically by `(city, product)`: `Accra|B`, then `Tema|A`, then `Tema|B`.
`sales` is untouched by this call — it isn't one of the tallied key features.

Example 2. Tally a single feature.

```cpp
auto by_city = orders.tally("city");
std::cout << by_city;
```

### Output

```text
===================
     | city   count
-------------------
0    | Accra  3    
1    | Tema   2    
-------------------
[2 cs x 2 feat]
```

Example 3. Tally over every feature — including a numeric one, compared as text.

```cpp
auto by_row = orders.tally();
std::cout << by_row;
```

### Output

```text
===================================
     | city   product  sales  count
-----------------------------------
0    | Accra  A        100    1    
1    | Accra  A        95     1    
2    | Accra  B        120    1    
3    | Tema   A        90     1    
4    | Tema   B        150    1    
-----------------------------------
[5 cs x 4 feat]
```

Every combination of `(city, product, sales)` is unique here, so every row's count
is `1`, and the tie-break sort falls back entirely to alphabetical key order.
Notice `sales`'s text-comparison ordering within the `Accra`/`A` group: `"100"`
sorts before `"95"`, because tally's key comparison is lexicographic (character by
character) on the stringified value, not a numeric comparison — `"1"` is less than
`"9"` as the very first character, regardless of the numbers' actual magnitude.

## Notes

- Key columns in the result are always `Note` (text), even when the source feature
  was numeric — sorting and tie-breaking both happen on the stringified value, which
  can order numbers lexicographically rather than numerically (see Example 3).
- `top_n` truncation happens **after** sorting by count descending, so it always
  keeps the highest-count rows, never an arbitrary prefix.
- With `dropnull = false`, a missing key value becomes the literal text `"NULL"`
  rather than being excluded from the tally.

## See Also

- [`DataFrame::freq`](freq.md): the `Thread`-returning sibling, packing the key into
  one composite label instead of separate columns.
- [`DataFrame::duplicated`](duplicated.md), [`DataFrame::repeated`](repeated.md):
  flag individual rows by the same composite-key convention, rather than counting
  combinations.
- [`DataFrame::count_by`](count_by.md): count rows grouped by exactly one feature,
  a narrower convenience this method's single-feature overload also covers.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="title.html">
    <span class="label">Next</span>
    <span class="title">title</span>
  </a>
</div>
