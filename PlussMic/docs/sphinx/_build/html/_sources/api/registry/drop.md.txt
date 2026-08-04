<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e67d1621ad06 -->

# Registry::drop

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry Registry::drop(std::string_view key) const
Registry Registry::drop(std::span<const std::string_view> keys) const
Registry Registry::drop(std::initializer_list<std::string_view> keys) const
Registry Registry::drop(const std::vector<std::string>& keys) const
template <pluss::detail::StringViewNameRange NameRange>
Registry Registry::drop(const NameRange& keys) const
```

## What It Does

Returns a new registry with the named field(s) removed, keeping every other field
in its original order. The complement of [`fields(keys)`](fields.md) — `fields(keys)`
keeps only the named fields, `drop(keys)` keeps everything *except* them.

```{important} An unknown key throws — drop() does not silently ignore it
Every overload validates all requested keys against the registry's fields before
removing anything: a name that doesn't exist throws `"Registry::drop: unknown key '<name>'"`.
This is the same fail-fast behavior [`fields(keys)`](fields.md) has, and the opposite
of [`has_all()`](has_all.md)/[`has_any()`](has_any.md), which report missing keys
as `false` rather than throwing.
```

## Parameters

**key** (`std::string_view`)
    A single field name to remove (single-key overload).

**keys** (`std::span<const std::string_view>` / `std::initializer_list<std::string_view>` / `const std::vector<std::string>&` / any string-view name range)
    Multiple field names to remove.

## Returns

`Registry`
    A new registry with the named field(s) removed and every other field
    preserved in its original order.

## Examples

Example 1. Drop a single field.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"population", pluss::bundle::Slot({pluss::make_series(100), pluss::make_series(200), pluss::make_series(300)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop("city");
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id population
```

Example 2. Drop several fields at once (initializer-list overload).

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"population", pluss::bundle::Slot({pluss::make_series(100), pluss::make_series(200), pluss::make_series(300)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop({"population", "city"});
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id
```

Example 3. Dropping a field that doesn't exist throws — nothing is removed.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
});

try {
  auto out = rg.drop("zzz");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::drop: unknown key 'zzz'
```

## Notes

- Throws on any unknown key — see the important callout above. This applies to
  every overload, including passing a container from a variable (as in Example
  3's single-key form).
- Preserves the original field order of everything not dropped.
- Never mutates the receiver — always returns a new `Registry`.

## See Also

- [`Registry::fields`](fields.md): the complementary keep-only operation.
- [`Registry::prune`](prune.md): a related but distinct operation — drops fields
  by *emptiness*, not by name.
- [`Registry::has`](has.md) / [`Registry::has_all`](has_all.md): non-throwing
  presence checks, useful for validating keys before calling `drop()` if you're
  unsure they exist.
- [`Catalog::drop`](../catalog/drop.md): the equivalent operation on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="distinct.html">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
