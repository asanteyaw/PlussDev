<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a00d60879f5b -->

# DataFrameLabelAccessor::rename

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::rename(pluss::msharp::RangeView::Of<DataFrame::LabelRename> renames) const -> DataFrame
DataFrameLabelAccessor::rename(std::initializer_list<DataFrame::LabelRename> renames) const -> DataFrame
```

## What It Does

Renames one or more case-label levels by explicit `{from, to}` pairs. Unlike
[`names()`](names.md)'s renaming overloads, you don't have to rename every
level — just the ones you list. Requires a structured (multi-level) frame;
values, row order, and the displayed composite label text are all unaffected —
only the level *names* returned by [`names()`](names.md) (and usable with
[`part(name)`](part.md)) change.

## Parameters

**renames** (`pluss::msharp::RangeView::Of<DataFrame::LabelRename>` /
`std::initializer_list<DataFrame::LabelRename>`)

One or more `{from, to}` pairs — each `from` must name an existing level, and
no two renames may target the same `to`.

## Returns

`DataFrame`
    A new frame with the named levels renamed; values and row order unchanged.

## Examples

Example 1. Rename one level of a 2-level structured frame, leaving the other
untouched.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Slate(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});
auto grouped = readings.label().set({"region", "city"}, true, "/");
std::cout << grouped;

for (const auto& name : grouped.label().names()) {
  std::cout << name << " ";
}
```

### Output (grouped)

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

### Output (level names, before renaming)

```text
region city 
```

```cpp
auto renamed = grouped.label().rename({{"region", "area"}});
std::cout << renamed;

for (const auto& name : renamed.label().names()) {
  std::cout << name << " ";
}
```

### Output (renamed)

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

### Output (level names, after renaming)

```text
area city 
```

Only `"region"` was renamed to `"area"`; `"city"` is left exactly as it was.
`renamed.label().part("area")` now reads what used to be `part("region")`;
the printed table is identical before and after — `rename()` only affects
level metadata, never the values or their join.

Example 2. Rename both levels at once, with two `{from, to}` pairs in a single
call.

```cpp
auto renamed_both = grouped.label().rename({{"region", "area"}, {"city", "town"}});

for (const auto& name : renamed_both.label().names()) {
  std::cout << name << " ";
}
```

### Output

```text
area town 
```

A single `rename()` call isn't limited to one pair — listing `{"region",
"area"}` and `{"city", "town"}` together renames both levels in one step,
same as chaining two single-pair calls would.

Example 3. Call `rename()` on an unstructured frame, to see it throw.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"sales", Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});

try {
  auto out = orders.label().rename({{"region", "area"}});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::rename_caselbls: current DataFrame has no case levels
```

`orders` was never promoted to structured case labels via `set()`, so there's
no `"region"` level to rename.

Example 4. Rename two different levels to the same target name, to see a
different throw.

```cpp
try {
  auto out = grouped.label().rename({{"region", "area"}, {"city", "area"}});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::rename_caselbls: duplicate target case label 'area'
```

Both `"region"` and `"city"` are valid, existing levels, but renaming them
both to `"area"` would collapse two distinct levels into one name — that's
rejected rather than silently allowed.

## Notes

- Every `from` must name an existing level; every `to` must be unique across
  the whole rename set (you can't collapse two levels into the same name).
- Throws if the frame isn't structured — there are no levels to rename.

## See Also

- [`DataFrameLabelAccessor::names`](names.md): list current level names, or
  rename **all** of them positionally in one call.
- [`DataFrameLabelAccessor::order`](order.md),
  [`DataFrameLabelAccessor::swap`](swap.md): reorder levels rather than rename
  them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pos.html">
    <span class="label">Previous</span>
    <span class="title">pos</span>
  </a>
  <a class="next-link" href="repeated.html">
    <span class="label">Next</span>
    <span class="title">repeated</span>
  </a>
</div>
