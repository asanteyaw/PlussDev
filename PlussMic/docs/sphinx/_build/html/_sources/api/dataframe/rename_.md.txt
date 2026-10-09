<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1462bd28be17 -->

# DataFrame::rename_

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame& DataFrame::rename_(std::string_view old_name, std::string new_name)
```

## What It Does

The in-place counterpart to [`rename()`](rename.md)'s targeted single-name overload:
renames exactly one feature, mutating `*this` directly and returning a reference to
it, rather than returning a new frame. Values, dtype, and case labels are untouched —
only the feature's name changes.

Unlike `rename()`, which offers several overloads (single pair, multiple pairs, or a
full positional-list replace), `rename_()` only supports the single-feature form —
there is no in-place equivalent of `rename()`'s multi-pair or full-axis-replace
overloads.

## Parameters

**old_name** (`std::string_view`)

The existing feature name to rename. Must already exist on the frame.

**new_name** (`std::string`)

The replacement name. Must not already be used by another feature on the frame
(unless it is identical to `old_name`, which is a no-op).

## Returns

`DataFrame&`
    A reference to `*this`, with `old_name` renamed to `new_name`.

## Examples

Example 1. Rename one feature in place.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Slate(pluss::make_series({1, 2, 3}))},
    {"sales", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

orders.rename_("sales", "revenue");
std::cout << orders;
```

### Output

```text
============================
     | id    revenue  city  
----------------------------
0    | 1     100      Accra 
1    | 2     120      Tema  
2    | 3     90       Kumasi
----------------------------
[3 cs x 3 feat]
```

`orders` itself is renamed — every value, dtype, and case label is unchanged, only
the column header text differs.

## Notes

- `rename_()` throws if `old_name` doesn't exist, or if `new_name` collides with a
  different, already-existing feature name.
- Renaming a feature to its own current name (`old_name == new_name`) is a harmless
  no-op.
- For renaming several features at once, or replacing every feature name
  positionally, use [`rename()`](rename.md) — `rename_()` only covers the
  single-feature, in-place case.

## See Also

- [`DataFrame::rename`](rename.md): the non-mutating counterpart, with additional
  overloads for multi-pair and full-axis renames.
- [`DataFrame::label`](label.md): the case-label accessor, for renaming or otherwise
  editing row labels instead of feature names.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename.html">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="reorder.html">
    <span class="label">Next</span>
    <span class="title">reorder</span>
  </a>
</div>
