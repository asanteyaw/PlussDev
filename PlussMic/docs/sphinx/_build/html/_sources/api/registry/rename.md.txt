<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f5ae6afbc8bf -->

# Registry::rename

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry Registry::rename(std::string_view from, std::string_view to) const
Registry Registry::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const
Registry Registry::rename(const std::unordered_map<std::string, std::string>& mapping) const
```

## What It Does

Returns a new registry with one or more fields renamed, in place in the field
order — values are untouched, only the field names change. The single `from`/`to`
overload renames exactly one field; the `mapping` overloads (an initializer list of
`{from, to}` pairs, or an `unordered_map`) rename several fields in one call.

```{important} Both overloads validate differently, and rename() never overwrites your own mistake
The initializer-list overload rejects a **duplicate source key** within the same
call (`"Registry::rename: duplicate source key in mapping"`) before it even
reaches the shared rename logic. Both overloads (after that point) reject any
target name that collides with another field's final name —
`"Registry::rename: duplicate target key '<name>'"` — including a target that
collides with a field that wasn't itself being renamed. An unknown source key
throws `"Registry::rename: unknown key '<name>'"`, and an empty target name throws
too. An **empty mapping** (`unordered_map` overload only) is the one case that
succeeds trivially, returning the registry unchanged.
```

## Parameters

**from** / **to** (`std::string_view`)
    The existing field name and its replacement (single-pair overload).

**mapping** (`std::initializer_list<std::pair<std::string_view, std::string_view>>` / `const std::unordered_map<std::string, std::string>&`)
    A `{from, to}` mapping for renaming several fields in one call.

## Returns

`Registry`
    A new registry with the requested field(s) renamed; every other field keeps
    its original name.

## Examples

Example 1. Rename several fields at once via an initializer list of pairs.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"population", pluss::scribble::Slot({pluss::make_series(100), pluss::make_series(200), pluss::make_series(300)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.rename({{"city", "location"}, {"id", "identifier"}});
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
identifier population location
```

Example 2. The same rename via an `unordered_map`.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra")})},
});

std::unordered_map<std::string, std::string> mapping{{"city", "location"}};
auto out = rg.rename(mapping);
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id location
```

Example 3. The single-pair overload for renaming exactly one field.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra")})},
});

auto out = rg.rename("city", "location");
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id location
```

Example 4. Renaming to a name that already belongs to another field throws.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra")})},
});

try {
  auto out = rg.rename("city", "id");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::rename: duplicate target key 'id'
```

## Notes

- Field order is preserved — a renamed field keeps its original position, only
  its name changes.
- See the important callout above for the exact throw messages and the empty-mapping
  special case.
- Never mutates the receiver — use [`rename_()`](rename_.md) for the in-place
  form.

## See Also

- [`Registry::rename_`](rename_.md): the in-place counterpart (single-pair form
  only).
- [`Registry::combine`](combine.md): auto-generates suffixed names on collision,
  rather than throwing — useful when you want automatic collision handling
  instead of manual renaming.
- [`Catalog::rename`](../catalog/rename.md): the equivalent operation on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="push_.html">
    <span class="label">Previous</span>
    <span class="title">push_</span>
  </a>
  <a class="next-link" href="rename_.html">
    <span class="label">Next</span>
    <span class="title">rename_</span>
  </a>
</div>
