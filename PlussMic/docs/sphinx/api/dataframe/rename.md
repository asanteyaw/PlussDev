<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 33da707573bd -->

# DataFrame::rename

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::rename(std::string_view old_name, std::string new_name) const -> DataFrame
DataFrame::rename(pluss::RangeView<std::pair<std::string_view, std::string_view>> renames) const -> DataFrame
DataFrame::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> renames) const -> DataFrame
DataFrame::rename(pluss::RangeView<std::string_view> feature_names) const -> DataFrame
DataFrame::rename(std::initializer_list<const char*> feature_names) const -> DataFrame
```

## What It Does

Changes feature names without touching a single value, dtype, or case label. Two
distinct shapes are supported, and picking the right one matters:

- **Targeted rename** (`old_name`/`new_name`, or a collection of `(old_name, new_name)`
  pairs): rename one or a few specific features by name, leaving every other feature's
  name untouched. This is the overload to reach for almost all the time.
- **Full-axis replace** (a plain list of names, with no pairing): replace *every* feature
  name at once, positionally, left to right. The list you pass must have exactly
  `nfeatures()` entries, in the exact order the current features already appear — this
  overload does not know or care what the old names were, it only knows position.

Mixing these up is the most common mistake: passing a short list intending a targeted
rename will instead try to rename *every* feature positionally (and fail or misname
things if the count doesn't match). Use the pair-based overloads unless you genuinely
mean "replace the entire schema in one shot."

## Parameters

**old_name** (`std::string_view`)

Existing feature name to rename, for the single-rename overload.

**new_name** (`std::string`)

Replacement name for `old_name`.

**renames** (`pluss::RangeView<std::pair<std::string_view, std::string_view>>` /
`std::initializer_list<std::pair<std::string_view, std::string_view>>`)

A collection of `(old_name, new_name)` pairs, applied together. Features not named in
`renames` keep their current name.

**feature_names** (`pluss::RangeView<std::string_view>` /
`std::initializer_list<const char*>`)

A full replacement name list, applied **positionally** to every feature on the frame,
left to right. Must have exactly `nfeatures()` entries.

## Returns

`DataFrame`
    A new frame with the requested names applied. Feature order, values, dtypes, and the
    case axis are all unchanged — only names move.

## Examples

Example 1. Rename two specific features, leaving the rest alone.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"order_id", Thread(pluss::make_series({101, 102, 103}))},
    {"sales",    Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",     Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename({{"sales", "revenue"}, {"city", "location"}});
std::cout << out;
```

### Output

```text
==================================
     | order_id  revenue  location
----------------------------------
0    | 101       100      Accra   
1    | 102       120      Tema    
2    | 103       90       Kumasi  
----------------------------------
[3 cs x 3 feat]
```

`order_id` was never mentioned in the rename pairs, so it keeps its original name.

Example 2. Rename a single feature with the shorthand two-argument overload.

```cpp
auto out = df.rename("sales", "revenue");
std::cout << out;
```

### Output

```text
================================
     | order_id  revenue  city  
--------------------------------
0    | 101       100      Accra 
1    | 102       120      Tema  
2    | 103       90       Kumasi
--------------------------------
[3 cs x 3 feat]
```

Equivalent to `df.rename({{"sales", "revenue"}})`, but reads more directly when you only
have one feature to rename.

Example 3. Replace every feature name at once, positionally.

```cpp
auto out = df.rename({"id_new", "revenue", "location"});
std::cout << out;
```

### Output

```text
================================
     | id_new  revenue  location
--------------------------------
0    | 101     100      Accra   
1    | 102     120      Tema    
2    | 103     90       Kumasi  
--------------------------------
[3 cs x 3 feat]
```

All three names changed here, including `order_id` -> `id_new` — this overload has no
concept of matching by the *old* name; `{"id_new", "revenue", "location"}` is applied
purely by position, so it must list exactly as many names as `df` has features, in the
frame's current left-to-right feature order.

## Notes

- The full-axis overload (a plain list, no pairing) requires the list length to match
  `nfeatures()` exactly. A shorter or longer list is a programming error, not something
  `rename()` pads or truncates for you.
- `rename()` never changes values, dtypes, feature order, or case labels — it is a
  pure schema-naming operation.
- For an in-place variant on a single feature, see [`rename_()`](rename_.md) (note the
  trailing underscore, the project-wide convention for mutating siblings of a
  non-mutating method).

## See Also

- [`DataFrame::rename_`](rename_.md): the in-place, mutating single-feature rename.
- [`DataFrame::drop`](drop.md): remove features instead of renaming them.
- [`DataFrame::features`](features.md): read the current feature names before deciding
  how to rename them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rdiv.html">
    <span class="label">Previous</span>
    <span class="title">rdiv</span>
  </a>
  <a class="next-link" href="rename_.html">
    <span class="label">Next</span>
    <span class="title">rename_</span>
  </a>
</div>
