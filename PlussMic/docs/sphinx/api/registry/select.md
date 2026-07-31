<!-- pluss-hand-authored -->

# Registry::select

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Slot Registry::select(std::string_view key) const
Registry Registry::select(std::span<const std::string_view> keys) const
Registry Registry::select(std::initializer_list<std::string_view> keys) const
Registry Registry::select(const std::vector<std::string>& keys) const
template <pluss::detail::StringViewNameRange NameRange>
Registry Registry::select(const NameRange& keys) const
```

## What It Does

`select()` plays **two distinct roles** depending on the argument shape, much like
[`fields()`](fields.md):

- **Single-key overload** — `select(key)` returns that field's `Slot` **directly**,
  not a one-field `Registry`. This is the direct-fetch role, equivalent to (and
  in fact implemented as) the private `slot(key)` lookup.
- **Multi-key overloads** — `select(keys)` (an initializer list, a `span`, a
  `std::vector<std::string>`, or any string-view-convertible range) returns a new
  `Registry` containing only the named fields, in the order requested.

```{important} select(key) and select(keys) return different types
`select("id")` returns a `Slot`; `select({"id"})` (note the braces) returns a
`Registry` with one field named `"id"`. These are easy to conflate at a glance —
the single-argument, brace-less call is the one that skips straight to the `Slot`.
```

## Parameters

**key** (`std::string_view`)
    A single field name to fetch directly (single-key overload).

**keys** (`std::span<const std::string_view>` / `std::initializer_list<std::string_view>` / `const std::vector<std::string>&` / any string-view name range)
    Multiple field names to keep, in the given order.

## Returns

`Slot` (single-key overload)
    The named field's `Slot` directly.

`Registry` (multi-key overloads)
    A new registry containing only the named fields, in the order requested.

## Examples

Example 1. The multi-key overload returns a new registry with only the named
fields, reordered to match the request.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"population", pluss::bundle::Slot({pluss::make_series(100), pluss::make_series(200), pluss::make_series(300)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.select({"city", "id"});
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
city id
```

Example 2. The single-key overload returns the `Slot` directly.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

auto ids = rg.select("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[1, 2, 3]
```

Example 3. Requesting an unknown field throws, in either form.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
});

try {
  auto out = rg.select({"zzz"});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::select: unknown key 'zzz'
```

## Notes

- See the important callout above for the type-changing overload split — the same
  duality [`fields()`](fields.md) has.
- Unlike [`get()`](get.md), which returns `std::optional<Slot>` for a missing key,
  every `select()` overload **throws** on an unknown key.
- Requesting fields in a different order than they appear in the registry
  reorders the result to match the request (Example 1).
- Never mutates the receiver — always returns a new `Registry` or `Slot`.

## See Also

- [`Registry::get`](get.md): the non-throwing, `optional`-returning alternative to
  the single-key overload.
- [`Registry::drop`](drop.md): the complementary keep-everything-except operation.
- [`Registry::fields`](fields.md): a similarly dual-role method, for field names
  and the field-scoped accessor rather than field values.
- [`Catalog::select`](../catalog/select.md): the equivalent operation on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_txt.html">
    <span class="label">Previous</span>
    <span class="title">save_txt</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
