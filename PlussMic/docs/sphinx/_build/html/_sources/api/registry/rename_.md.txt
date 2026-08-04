<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f0fdaaab993c -->

# Registry::rename_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry& Registry::rename_(std::string_view from, std::string_view to)
```

## What It Does

The in-place counterpart to the single-pair form of [`rename()`](rename.md) —
renames exactly one field on this registry directly. Implemented as
`*this = rename(from, to)`. There is no in-place overload for the multi-field
mapping forms; call `rename_()` repeatedly, or use [`rename()`](rename.md)'s
mapping overloads and reassign, if you need to rename several fields in place.

## Parameters

**from** (`std::string_view`)
    The field's current name.

**to** (`std::string_view`)
    The field's new name.

## Returns

`Registry&`
    A reference to this registry, with the named field renamed.

## Examples

Example 1. Rename a field in place.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra")})},
});

rg.rename_("city", "location");
for (const auto& key : rg.fields()) std::cout << key << " ";
```

### Output

```text
id location
```

## Notes

- Implemented as `*this = rename(from, to)` — full-registry reassignment under the
  hood, not a field-by-field in-place rename.
- Only the single-pair form has an in-place overload; the initializer-list and
  `unordered_map` mapping overloads exist only on [`rename()`](rename.md).
- Same throw conditions as `rename()`'s single-pair overload — an unknown source
  key, an empty target name, or a target name colliding with another field all
  throw.

## See Also

- [`Registry::rename`](rename.md): the non-mutating counterpart, including the
  multi-field mapping overloads this method doesn't have.
- [`Catalog::rename_`](../catalog/rename_.md): the equivalent operation on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename.html">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="sample.html">
    <span class="label">Next</span>
    <span class="title">sample</span>
  </a>
</div>
