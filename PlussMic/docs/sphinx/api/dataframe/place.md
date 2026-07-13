# DataFrame::place

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::place(std::string name, const Thread& feature, std::string_view anchor_feature, bool after = false) -> DataFrame

DataFrame::place(std::string_view name, std::string_view anchor_feature, bool after = false) -> DataFrame
```

## What It Does

Moves one existing feature relative to an anchor feature, or inserts a brand-new feature directly before or after that anchor.

## Parameters

**name**
    Feature name to insert or move.

**feature**
    Full thread payload to insert at the target position. Its length must match the frame case count.

**anchor_feature**
    Reference feature used to place a newly inserted or moved feature before or after an existing column.

**after**
    When `false`, placement happens before the anchor. When `true`, placement happens after the anchor.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Insert a new feature relative to an anchor feature without rebuilding the whole schema.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.place("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})), "sales", true);
```

### Output

```text
id  sales  profit  city
1   100.0  40.0    Accra
2   120.0  35.0    Tema
3    90.0  30.0    Kumasi
```
Example 2. Move one existing feature relative to an anchor feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.place("city", "id", false);
```

### Output

```text
city    id  sales
Accra   1   100.0
Tema    2   120.0
Kumasi  3    90.0
```

## Practical Behavior

- `place(name, feature, anchor, after)` inserts a brand-new feature relative to an existing anchor.
- `place(name, anchor, after)` moves an existing feature without forcing you to restate the full feature order.
- When `after=false`, placement happens before the anchor. When `after=true`, placement happens after the anchor.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pivot_table.html">
    <span class="label">Previous</span>
    <span class="title">pivot_table</span>
  </a>
  <a class="next-link" href="plot.html">
    <span class="label">Next</span>
    <span class="title">plot</span>
  </a>
</div>
