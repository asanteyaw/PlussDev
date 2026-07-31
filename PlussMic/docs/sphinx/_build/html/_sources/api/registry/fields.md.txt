<!-- pluss-hand-authored -->

# Registry::fields

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
const std::vector<std::string>& Registry::fields() const
RegistryFieldAccessor Registry::fields(std::string_view key) const
RegistryFieldAccessor Registry::fields(std::span<const std::string_view> keys) const
RegistryFieldAccessor Registry::fields(std::initializer_list<std::string_view> keys) const
template <pluss::detail::StringViewNameRange NameRange>
RegistryFieldAccessor Registry::fields(const NameRange& keys) const
```

## What It Does

`fields()` plays **two distinct roles** depending on whether you pass an argument:

- **No-arg overload** — `fields()` is a plain getter, returning the registry's field
  names in field order. This is the identity/introspection role, analogous to
  `Catalog::keys()`.
- **Argument overloads** — `fields(key)` / `fields(keys)` bind the registry and a
  selection of field names into a `RegistryFieldAccessor`, the entry point for
  field-scoped operations (`cast()`, `nt()`, `tl()`, plus null-handling and
  distinct/frequency shorthands). This is the accessor-launcher role.

```{important} fields() is overloaded across two unrelated purposes
Calling `fields()` with no arguments and calling `fields(key)` with one do
fundamentally different things — the first returns `const std::vector<std::string>&`
(the field names), the second returns a `RegistryFieldAccessor` (a binder for
further operations on the named field). Reading call sites carefully for the
presence or absence of an argument matters here.
```

## Parameters

**key** (`std::string_view`)
    A single field name to bind the accessor to.

**keys** (`std::span<const std::string_view>` / `std::initializer_list<std::string_view>` / any string-view name range)
    Multiple field names to bind the accessor to at once.

## Returns

`const std::vector<std::string>&` (no-arg overload)
    The registry's field names, in field order.

`RegistryFieldAccessor` (argument overloads)
    A binder for the current registry and the selected field name(s) — call one of
    its methods to actually transform or query those fields.

## Examples

Example 1. The no-arg overload returns the field names.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

for (const auto& name : rg.fields()) std::cout << name << " ";
```

### Output

```text
id city
```

Example 2. The argument overload binds an accessor over the named fields — here
using its `dropnull()` shorthand, which returns a full `Registry`, not just the
selected fields.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fields({"id", "city"}).dropnull();
std::cout << out.nfields();
```

### Output

```text
2
```

## Notes

- The no-arg overload returns a reference into the registry's own storage — valid
  as long as the registry itself is alive.
- The accessor-launcher overloads always return a new `RegistryFieldAccessor` that
  binds a **copy** of the registry handle (cheap — `Registry` shares storage) plus
  the selected field names; the accessor's own methods do the actual work and are
  documented on their own pages.
- `fields(key)` accepts a single name directly; there is no separate single-name
  accessor type — it's the same `RegistryFieldAccessor` as the multi-name overloads,
  just bound to one field.

## See Also

- [`RegistryFieldAccessor` API Reference](../../registry_field_accessor_api_reference):
  the full method surface reachable from the argument overloads.
- [`Registry::fieldaxis`](fieldaxis.md): select fields by name pattern or value kind
  instead of an explicit list.
- [`Registry::select`](select.md): return the selected fields' values directly as a
  new `Registry`, without going through an accessor.
- [`Catalog::keys`](../catalog/keys.md): the equivalent no-arg name getter on the
  kindle model (which has no accessor-launcher counterpart, since `Catalog` streams
  are already uniformly typed).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fieldaxis.html">
    <span class="label">Previous</span>
    <span class="title">fieldaxis</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
