<!-- pluss-hand-authored -->

# DataFrame::features

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::features() const -> const std::vector<std::string>&
DataFrame::features(std::string_view name) const -> Thread
DataFrame::features(std::initializer_list<std::string_view> names) const -> DataFrame
DataFrame::features(pluss::msharp::RangeView::Of<std::string_view> names) const -> DataFrame
DataFrame::features(StackArray<std::string_view> names) const -> DataFrame
```

## What It Does

Three genuinely different behaviors share this one overloaded name, distinguished
entirely by how many names you pass:

- **No arguments** — `features()` returns the frame's ordered list of feature names,
  as a plain `const std::vector<std::string>&`.
- **One name** — `features("sales")` extracts a *single* feature as a `Thread`, with
  its case labels and display name attached (so printing it shows a
  `Name: sales, ptype: Series`-style footer, exactly like other named `Thread`
  results).
- **Several names** — `features({"id", "sales"})` (or the equivalent range/
  `StackArray` overloads) projects the frame down to a **subset** `DataFrame`
  containing just those columns, in the order you named them, with every case
  preserved.

Every non-trivial name lookup throws if the name doesn't exist on the frame.

## Parameters

**name** (`std::string_view`, single-feature overload)

The one feature to extract as a standalone `Thread`.

**names** (`initializer_list`/`RangeView`/`StackArray` of `std::string_view`,
multi-feature overload)

The features to project, in the order you list them — the result's feature order
follows this list, not the source frame's original order.

## Returns

`const std::vector<std::string>&` (no-argument overload)
    The frame's feature names, in their current order.

`Thread` (single-name overload)
    The named feature, with case labels and display name attached.

`DataFrame` (multi-name overload)
    A new frame containing only the named features, in the order requested, with
    every case preserved.

## Examples

Example 1. List every feature name.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    pluss::make_series({1, 2, 3})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"city",  pluss::make_note({"Accra", "Tema", "Kumasi"})},
}, pluss::make_note({"p1", "p2", "p3"}));

std::cout << orders;
```

### Output (orders)

```text
==========================
     | id    sales  city  
--------------------------
p1   | 1     100    Accra 
p2   | 2     120    Tema  
p3   | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

```cpp
for (const auto& name : orders.features()) {
  std::cout << name << " ";
}
```

### Output

```text
id sales city
```

Example 2. Extract a single feature as a standalone `Thread`.

```cpp
auto sales_col = orders.features("sales");
std::cout << sales_col;
```

### Output

```text
p1  100
p2  120
p3  90
Name: sales, ptype: Series
```

Unlike a raw column pulled some other way, `sales_col` carries `orders`'s case labels
(`p1, p2, p3`) and its own name (`sales`) for display — the footer shows both.

Example 3. Project a named subset into a new frame.

```cpp
auto subset = orders.features({"id", "sales"});
std::cout << subset;
```

### Output

```text
==================
     | id    sales
------------------
p1   | 1     100  
p2   | 2     120  
p3   | 3     90   
------------------
[3 cs x 2 feat]
```

`city` is dropped entirely; `id` and `sales` keep their original values and case
labels, in the order requested. Listing names in a different order (say,
`{"sales", "id"}`) would reorder the result's columns to match.

## Notes

- All three overload families throw if any requested name doesn't exist on the frame.
- The multi-name overload's result order follows the order you list names in, not the
  source frame's order — use this to reorder columns as a side effect of selecting
  them.
- The single-name overload's returned `Thread` carries the source frame's case labels
  and the feature's own name for display; the no-argument overload returns a plain
  name list with no such attachment.

## See Also

- [`DataFrame::nfeatures`](nfeatures.md): the count matching this list's length.
- [`DataFrame::get`](get.md): a null-safe alternative to the single-name overload,
  returning `std::optional<Thread>` instead of throwing on an unknown name.
- [`DataFrame::drop`](drop.md): the complementary operation — remove named features
  instead of projecting down to them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="feataxis.html">
    <span class="label">Previous</span>
    <span class="title">feataxis</span>
  </a>
  <a class="next-link" href="ffill.html">
    <span class="label">Next</span>
    <span class="title">ffill</span>
  </a>
</div>
