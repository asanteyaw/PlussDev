<!-- pluss-hand-authored -->

# Registry::apply

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry Registry::apply(std::string_view key, const std::function<Slot(const Slot&)>& fn) const
Registry Registry::apply(std::span<const std::string_view> keys, const std::function<Slot(const Slot&)>& fn) const
Registry Registry::apply(std::initializer_list<std::string_view> keys, const std::function<Slot(const Slot&)>& fn) const
template <pluss::detail::StringViewNameRange NameRange>
Registry Registry::apply(const NameRange& keys, const std::function<Slot(const Slot&)>& fn) const
```

## What It Does

Runs `fn` over one or more named fields' `Slot`s and returns a **new** registry with
those fields replaced by `fn`'s result — every other field is left untouched. The
single-key overload transforms exactly one field; the `keys` overloads (an
initializer list, a `span`, or any string-view-convertible range — `std::vector<std::string>`,
`std::vector<std::string_view>`, etc.) transform each named field independently
with the same `fn`, called once per field.

```{important} fn must return a defined Slot
`apply()`'s callback must return a `Slot` for which `defined()` is `true` — an
undefined result throws `"Registry::apply_: function returned undefined Slot"`
(note the message names the internal `apply_` implementation `apply()` delegates
to, not `apply` itself). A default-constructed `Slot{}` returned from `fn` is the
most common way to trigger this by accident.
```

## Parameters

**key** (`std::string_view`)
    A single field name to transform (single-key overload).

**keys** (`std::span<const std::string_view>` / `std::initializer_list<std::string_view>` / any string-view name range)
    Multiple field names to transform, each independently, with the same `fn`.

**fn** (`const std::function<Slot(const Slot&)>&`)
    A callback taking the current field's `Slot` and returning its replacement.
    Must return a defined `Slot` — see the important callout above.

## Returns

`Registry`
    A new registry, identical to the original except the named field(s) are
    replaced by `fn`'s result.

## Examples

Example 1. Transform a single field — doubling every value in `population`.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"population", pluss::bundle::Slot({pluss::make_series(100), pluss::make_series(200), pluss::make_series(300)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply("population", [](const pluss::bundle::Slot& s) {
  return s.transform([](const pluss::bundle::Slot::Value& v) -> pluss::bundle::Slot::Value {
    return pluss::make_series(std::get<pluss::Series>(v).to_vec<std::int64_t>()[0] * 2);
  });
});

auto pop = out.select("population");
std::cout << "[";
for (std::int64_t i = 0; i < pop.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(pop.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[200, 400, 600]
```

Example 2. Transform several named fields at once (initializer-list overload) —
`apply()` never mutates the original, so `rg` is unaffected.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"population", pluss::bundle::Slot({pluss::make_series(100), pluss::make_series(200), pluss::make_series(300)})},
});

auto out = rg.apply({"id", "population"}, [](const pluss::bundle::Slot& s) {
  return s.transform([](const pluss::bundle::Slot::Value& v) -> pluss::bundle::Slot::Value {
    return pluss::make_series(std::get<pluss::Series>(v).to_vec<std::int64_t>()[0] + 1);
  });
});

auto id_out = out.select("id");
auto pop_out = out.select("population");
std::cout << std::get<pluss::Series>(id_out.at(0)).to_vec<std::int64_t>()[0] << " ";
std::cout << std::get<pluss::Series>(pop_out.at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
2 101
```

## Notes

- Never mutates the receiver — `apply()` always returns a new `Registry`; use
  [`apply_()`](apply_.md) for the in-place form.
- `fn` is called once per named field, independently — each field's `Slot` is
  passed and replaced on its own.
- The `keys` overloads accept an initializer list, a `span`, or any
  string-view-convertible range (`std::vector<std::string>`,
  `std::vector<std::string_view>`, etc.) — pick whichever matches the container you
  already have.
- To transform **every** field with the same function, use
  [`apply_all()`](apply_all.md)/[`map()`](map.md) instead of listing every field
  name.

## See Also

- [`Registry::apply_`](apply_.md): the in-place, mutating counterpart.
- [`Registry::apply_all`](apply_all.md) / [`Registry::map`](map.md): apply the same
  function to every field, without naming them.
- [`RegistryFieldAccessor`](../../registry_field_accessor_api_reference): a
  narrower, field-scoped alternative reached via [`fields(keys)`](fields.md) for
  the library's built-in per-field operations (fillnull, cast, text ops, etc.).
- [`Catalog::apply`](../catalog/apply.md): the equivalent operation on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="apply_.html">
    <span class="label">Next</span>
    <span class="title">apply_</span>
  </a>
</div>
