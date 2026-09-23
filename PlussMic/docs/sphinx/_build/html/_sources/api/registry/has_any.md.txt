<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2b286cee873c -->

# Registry::has_any

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
bool Registry::has_any(std::span<const std::string_view> keys) const
bool Registry::has_any(std::initializer_list<std::string_view> keys) const
template <pluss::detail::StringViewNameRange NameRange>
bool Registry::has_any(const NameRange& keys) const
```

## What It Does

Checks whether **at least one** name in `keys` exists as a field — the OR-combined
form of [`has()`](has.md) over multiple names at once.

## Parameters

**keys** (`std::span<const std::string_view>` / `std::initializer_list<std::string_view>` / any string-view name range)
    The field names to check.

## Returns

`bool`
    `true` if any name in `keys` is a field; `false` if none of them are (including
    when `keys` is empty, which vacuously returns `false`).

## Examples

Example 1. At least one requested name exists.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string_view> keys{"zzz", "city"};
std::cout << std::boolalpha << rg.has_any(keys);
```

### Output

```text
true
```

Example 2. None of the requested names exist.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string_view> keys{"zzz", "yyy"};
std::cout << std::boolalpha << rg.has_any(keys);
```

### Output

```text
false
```

## Notes

- An empty `keys` collection returns `false` (vacuous falsehood) — the mirror image
  of `has_all`'s vacuous-`true` convention on an empty collection.
- Key comparison is exact and case-sensitive, same as [`has()`](has.md).
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::has`](has.md): the single-key form this method combines with OR.
- [`Registry::has_all`](has_all.md): the AND-combined counterpart.
- [`Registry::fields`](fields.md): fetch multiple fields directly (multi-key
  overload), throwing if any is missing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_all.html">
    <span class="label">Previous</span>
    <span class="title">has_all</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
