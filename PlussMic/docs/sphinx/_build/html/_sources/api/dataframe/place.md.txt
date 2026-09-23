<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e4ce27e0a644 -->

# DataFrame::place

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::place(std::string name,
                 const Thread& feature,
                 std::string_view anchor_feature,
                 bool after = false) const -> DataFrame
DataFrame::place(std::string_view name,
                 std::string_view anchor_feature,
                 bool after = false) const -> DataFrame
```

## What It Does

Positions one feature relative to another named anchor feature, either inserting a
brand-new column there or moving an existing one — without having to restate the
frame's entire column order the way [`reorder()`](reorder.md)'s explicit-list
overload would. The first overload's `name` may or may not already exist on the
frame: if it doesn't, `place()` inserts `feature` as a new column next to `anchor`;
if it does, the existing column is replaced in place with `feature`'s values and,
if it wasn't already adjacent to `anchor`, moved there. The second overload always
moves an existing feature (by name only, keeping its current values) relative to
`anchor_feature`.

## Parameters

**name** (`std::string` or `std::string_view`)

The feature to insert or move. For the first overload, a name not already on the
frame inserts `feature` as new; a name that already exists replaces that column's
values with `feature` and relocates it next to `anchor_feature`.

**feature** (`const Thread&`, first overload only)

The column data to write at the target position. Its length must match the frame's
case count (when the frame already has features).

**anchor_feature** (`std::string_view`)

The existing feature `name` is placed immediately before or after. Must already
exist on the frame.

**after** (`bool`, default `false`)

`false` places `name` immediately **before** `anchor_feature`; `true` places it
immediately **after**.

## Returns

`DataFrame`
    A new frame with `name` inserted or relocated next to `anchor_feature`; every
    other feature keeps its relative order.

## Examples

Example 1. Insert a brand-new feature right after an existing one.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto profit = Thread(pluss::make_series({40.0, 35.0, 30.0}));
auto with_profit = orders.place("profit", profit, "sales", /*after=*/true);
std::cout << with_profit;
```

### Output

```text
==================================
     | id    sales  profit  city  
----------------------------------
0    | 1     100    40      Accra 
1    | 2     120    35      Tema  
2    | 3     90     30      Kumasi
----------------------------------
[3 cs x 4 feat]
```

`"profit"` didn't previously exist on `orders`, so `place()` inserts it as a new
column, positioned right after its anchor, `"sales"`.

Example 2. Move an existing feature next to a different anchor.

```cpp
auto city_first = orders.place("city", "id", /*after=*/false);
std::cout << city_first;
```

### Output

```text
==========================
     | city    id    sales
--------------------------
0    | Accra   1     100  
1    | Tema    2     120  
2    | Kumasi  3     90   
--------------------------
[3 cs x 3 feat]
```

`"city"` already exists, so this overload just relocates it — its values are
untouched, only its column position changes, landing immediately before its anchor,
`"id"`, since `after = false`.

## Notes

- With the first overload, if `name` already exists on the frame, `feature` replaces
  its current values in addition to relocating it — this overload can both rewrite
  and reposition a column in one call.
- `anchor_feature` must always already exist on the frame; an unknown anchor throws.
- Unlike [`reorder()`](reorder.md), `place()` only ever positions one feature at a
  time, relative to exactly one anchor — reach for `reorder()` when several features
  need to move in the same call.

## See Also

- [`DataFrame::reorder`](reorder.md): move several features at once, including
  before/after-anchor placement via `ReorderSpec`.
- [`DataFrame::insert`](insert.md): insert a new feature by positional index rather
  than by anchor name.
- [`DataFrame::drop`](drop.md): remove a feature entirely, the complement of
  inserting one with `place()`.

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
