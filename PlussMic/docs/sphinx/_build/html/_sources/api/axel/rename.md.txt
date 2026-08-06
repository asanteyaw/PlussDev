<!-- pluss-hand-authored -->

# Axel::rename

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const
Axel Axel::rename(std::string_view from, std::string_view to) const
```

## What It Does

Renames one field (the two-argument overload) or several fields at once
(the mapping overload) in every registry frame of the depth stack,
returning a new `Axel`. Implemented as [`map()`](map.md) applying the
matching `Registry::rename()` overload per frame.

## Parameters

**from** (`std::string_view`) / **to** (`std::string_view`)
    Single-field form: the existing field name and its replacement.

**mapping** (`std::initializer_list<std::pair<std::string_view, std::string_view>>`)
    Multi-field form: a list of `{from, to}` pairs to rename in one
    call.

## Returns

`Axel`
    A new axel with the named field(s) renamed in every frame, same
    depth as the original.

## Examples

Example 1. Rename `"ids"` to `"identifiers"` in every frame.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
});
auto out = axel.rename("ids", "identifiers");
std::cout << std::boolalpha << out.registry(0).has("identifiers");
```

### Output

```text
true
```

Example 2. Rename two fields in a single call using the mapping form.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({
        {"ids", pluss::bundle::Slot({pluss::make_series(1)})},
        {"city", pluss::bundle::Slot({pluss::make_note("Accra")})},
    }),
});
auto out = axel.rename({{"ids", "identifiers"}, {"city", "town"}});
std::cout << std::boolalpha << out.registry(0).has("identifiers") << " " << out.registry(0).has("town");
```

### Output

```text
true true
```

## Notes

- Whatever `Registry::rename()` throws for a missing `from` name in a
  given frame propagates the same way here.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"` (via the underlying `map()` call).

## See Also

- [`Axel::drop`](drop.md): removes fields outright instead of renaming
  them.
- [`Axel::map`](map.md): the general mechanism `rename()` is built on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="registry.html">
    <span class="label">Previous</span>
    <span class="title">registry</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
