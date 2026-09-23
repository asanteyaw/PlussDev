<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bf64274d1614 -->

# DataFrameLabelAccessor::names

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::names() const -> std::vector<std::string>
DataFrameLabelAccessor::names(pluss::msharp::RangeView::Of<std::string_view> names) const -> DataFrame
DataFrameLabelAccessor::names(std::initializer_list<std::string_view> names) const -> DataFrame
```

## What It Does

The no-argument overload lists the current case-label level names, in level
order — empty if the frame isn't structured. The `names`-taking overloads
**rename** every level in one call: you must supply exactly one new name per
existing level, positionally, and the result is exactly
[`rename()`](rename.md) applied with `{old_name, new_name}` pairs built from
that positional pairing.

## Parameters

**names** (renaming overloads)

One replacement name per existing level, in level order. Must match the
current level count exactly.

## Returns

`std::vector<std::string>`
    (No-argument overload) The current level names, in order; empty if
    unstructured.

`DataFrame`
    (`names`-taking overloads) A new frame with every level renamed.

## Examples

Example 1. List level names, then rename all of them at once.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"region", Thread(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Thread(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});
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
auto renamed = grouped.label().names({"area", "town"});
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
area town 
```

The printed table is identical before and after renaming — every value and
the displayed composite case label (`"east/Boston"`, and so on) is unchanged.
Only the level *names* returned by `names()` (and used by
[`part(name)`](part.md)) change, from `{"region", "city"}` to `{"area",
"town"}`.

Example 2. Pass fewer names than existing levels, to see the renaming
overload throw.

```cpp
try {
  auto out = grouped.label().names({"area"});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::label().names: expected one name per current label level
```

`grouped` has 2 levels; the renaming overload requires exactly one new name
per existing level, positionally, so a list of just one name throws instead
of leaving `"city"` renamed to something arbitrary.

## Notes

- The renaming overloads require **exactly** one name per existing level — you
  can't rename a subset; use [`rename()`](rename.md) for that.
- On an unstructured frame, `names()` (no arguments) returns an empty vector,
  and the renaming overloads have nothing to operate on.

## See Also

- [`DataFrameLabelAccessor::rename`](rename.md): rename a subset of levels by
  explicit `{from, to}` pairs.
- [`DataFrameLabelAccessor::part`](part.md): read a single level by its current
  name.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="monotonic.html">
    <span class="label">Previous</span>
    <span class="title">monotonic</span>
  </a>
  <a class="next-link" href="notnull.html">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>
