<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c191fc9fba2e -->

# DataFrame::duplicated

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::duplicated(pluss::RangeView<std::string_view> features = {}, bool keep_first = true, bool keep_last = false) const -> Thread
```

## What It Does

Builds a `Bool` mask, one entry per case, marking which rows are duplicates of an
earlier or later row when compared across the selected features. Rows are grouped by
the concatenated string value of every selected feature (missing cells compare as a
shared `"NULL"` placeholder, so two rows both missing the same feature are still
considered a match on that feature); any group with more than one row contributes
`true` entries according to `keep_first` / `keep_last`. This returns the mask itself —
for the frame with those rows already removed, see [`drop_duplicates()`](drop_duplicates.md).

`keep_first` and `keep_last` cannot both be `true` at the same time — asking to keep
both the first and last occurrence exempt from `true` is ambiguous when a group has
three or more rows, so the two flags are mutually exclusive.

## Parameters

**features** (`pluss::RangeView<std::string_view>`, default `{}`)

Which features to compare when deciding whether two rows match. An empty range (the
default) compares **every** column in the frame — including any unique identifier
column, which means no two rows will ever match if such a column is present. Pass the
specific feature names that define a "duplicate" for your use case (for example, just
`{"city", "product"}`, ignoring an `order_id` column that is unique per row by design).

**keep_first** (`bool`, default `true`)

When `true`, the first row encountered in each duplicate group is left `false` (not
flagged), and every later row in that group is flagged `true`.

**keep_last** (`bool`, default `false`)

When `true`, the last row encountered in each duplicate group is left `false`, and
every earlier row in that group is flagged `true`. Cannot be combined with
`keep_first = true`.

When both `keep_first` and `keep_last` are `false`, **every** row in a duplicate group
is flagged `true` — including what would otherwise be the exempted first or last row —
useful when you want to identify entire duplicate groups rather than just the "extra"
occurrences.

## Returns

`Thread`
    One `Bool` entry per case, labeled by case index and named `"duplicated"`,
    `true` wherever that row is considered a duplicate under the current
    `keep_first` / `keep_last` policy.

## Examples

Example 1. Flag every occurrence after the first one in each duplicate group — the
default policy.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",     Thread(pluss::make_note({"Accra", "Accra", "Tema", "Accra", "Kumasi", "Tema"}))},
    {"product",  Thread(pluss::make_note({"Laptop", "Laptop", "Phone", "Laptop", "Laptop", "Phone"}))},
});

auto dupes = orders.duplicated({"city", "product"});
std::cout << dupes;
```

### Output

```text
0  false
1  true
2  false
3  true
4  false
5  true
Name: duplicated, ptype: Series
```

Cases `0`, `1`, and `3` all share `(Accra, Laptop)`; case `0` is the first occurrence
and stays `false`, while `1` and `3` are flagged. Cases `2` and `5` share
`(Tema, Phone)`; case `2` stays `false`, case `5` is flagged. Case `4` (`Kumasi,
Laptop`) has no match anywhere else in the frame, so it is never flagged. `order_id`
was deliberately left out of `features` — including it would make every row unique
and nothing would ever be flagged.

Example 2. Use `keep_last = true` to exempt the *last* occurrence in each group
instead of the first.

```cpp
auto dupes_keep_last = orders.duplicated({"city", "product"}, false, true);
std::cout << dupes_keep_last;
```

### Output

```text
0  true
1  true
2  true
3  false
4  false
5  false
Name: duplicated, ptype: Series
```

The flags flip relative to Example 1: within `(Accra, Laptop)`, cases `0` and `1` are
now flagged and case `3` (the last one) is exempt; within `(Tema, Phone)`, case `2` is
flagged and case `5` (the last one) is exempt.

Example 3. Pass `false` for both flags to mark every row in a duplicate group,
including what would otherwise be the exempted first or last occurrence.

```cpp
auto dupes_all = orders.duplicated({"city", "product"}, false, false);
std::cout << dupes_all;
```

### Output

```text
0  true
1  true
2  true
3  true
4  false
5  true
Name: duplicated, ptype: Series
```

Every case belonging to a group with more than one member is now flagged `true` —
there is no exempted representative — while case `4` (the one group with a single
member) is still `false`, since a group of one is never a duplicate of anything.

## Notes

- `duplicated()`'s result is always named `"duplicated"` and labeled by case index,
  and prints like a small labeled `Series`, not like a row sliced from the source
  frame.
- Grouping is done by comparing string representations of the selected features'
  values, so it works uniformly across numeric, `Note`, `Timeline`, and `TimeIndex`
  features in the same call.
- Missing cells compare as a shared placeholder, so two rows that are both missing the
  same selected feature can still be grouped as duplicates of each other on that basis.
- `keep_first = true` and `keep_last = true` together is a validation error.

## See Also

- [`DataFrame::drop_duplicates`](drop_duplicates.md): removes the flagged rows instead
  of just marking them, sharing the exact same parameters.
- [`DataFrame::repeated`](repeated.md): a convenience shorthand equivalent to
  `duplicated(features, false, false)` — every occurrence in a duplicate group flagged.
- [`DataFrame::ndistinct`](ndistinct.md): counts unique combinations instead of
  flagging individual rows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dtypes.html">
    <span class="label">Previous</span>
    <span class="title">dtypes</span>
  </a>
  <a class="next-link" href="encode.html">
    <span class="label">Next</span>
    <span class="title">encode</span>
  </a>
</div>
