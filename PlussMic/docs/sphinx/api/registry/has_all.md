<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 688cd71778fe -->

# Registry::has_all

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
bool Registry::has_all(std::span<const std::string_view> keys) const
bool Registry::has_all(std::initializer_list<std::string_view> keys) const
template <pluss::detail::StringViewNameRange NameRange>
bool Registry::has_all(const NameRange& keys) const
```

## What It Does

Checks whether **every** name in `keys` exists as a field — the AND-combined form
of [`has()`](has.md) over multiple names at once.

## Parameters

**keys** (`std::span<const std::string_view>` / `std::initializer_list<std::string_view>` / any string-view name range)
    The field names to check.

## Returns

`bool`
    `true` if every name in `keys` is a field; `false` if any single one is missing
    (including when `keys` is empty, which vacuously returns `true`).

## Examples

Example 1. All requested names exist.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string_view> keys{"id", "city"};
std::cout << std::boolalpha << rg.has_all(keys);
```

### Output

```text
true
```

Example 2. One missing name is enough to fail the check.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string_view> keys{"id", "zzz"};
std::cout << std::boolalpha << rg.has_all(keys);
```

### Output

```text
false
```

## Notes

- An empty `keys` collection returns `true` (vacuous truth) — same convention as
  `has_all` checks elsewhere in the library.
- Key comparison is exact and case-sensitive, same as [`has()`](has.md).
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::has`](has.md): the single-key form this method combines with AND.
- [`Registry::has_any`](has_any.md): the OR-combined counterpart.
- [`Registry::fields`](fields.md): fetch multiple fields directly (multi-key
  overload), throwing if any is missing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="has_any.html">
    <span class="label">Next</span>
    <span class="title">has_any</span>
  </a>
</div>
