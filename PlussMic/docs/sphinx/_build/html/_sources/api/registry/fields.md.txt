<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e5c01407d3d4 -->

# Registry::fields

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
const std::vector<std::string>& Registry::fields() const
Slot Registry::fields(std::string_view key) const
Registry Registry::fields(std::span<const std::string_view> keys) const
Registry Registry::fields(std::initializer_list<std::string_view> keys) const
template <pluss::detail::StringViewNameRange NameRange>
Registry Registry::fields(const NameRange& keys) const
```

## What It Does

`fields()` has two roles:

- `fields()` with no argument returns the registry's field names.
- `fields("name")` returns one `Slot`.
- `fields(keys)` with a multi-name list returns a projected `Registry` containing only those fields, in the requested order.

This keeps `Registry` aligned with the table and kindle stacks:

- no-arg selector returns names
- one-name selector returns the leaf object
- multi-name selector returns the projected container

That gives you both direct slot work and projection-first registry work:

```cpp
auto slot = rg.fields("city").nt().upper();
```

```cpp
auto out = rg.fields({"city"}).nt().upper();
```

The first example returns a `Slot`. The second returns a one-field `Registry`.

If you want to rewrite one field while keeping the rest of the registry, fetch the slot, do the slot work, then reinsert with [`set`](set.md) or [`set_`](set_.md).

## Parameters

**key** (`std::string_view`)
    One field name to fetch as a `Slot`.

**keys** (`std::span<const std::string_view>` / `std::initializer_list<std::string_view>` / any string-view name range)
    The field names to keep in a projected registry, in the order you want them returned.

## Returns

`const std::vector<std::string>&`
    The registry's field names when called with no argument.

`Slot`
    The named slot when called with one field name.

`Registry`
    A projected registry containing only the requested fields when called with a name list.

## Examples

Example 1. Introspect field names.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

for (const auto& name : rg.fields()) std::cout << name << " ";
```

### Output

```text
id city
```

Example 2. Fetch one slot directly.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fields("city").nt().upper();
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
ACCRA
```

Example 3. Project one field, then run a note accessor on the projected registry.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fields({"city"}).nt().upper();
std::cout << out.nfields() << " " << std::get<pluss::Note>(out.fields("city").at(0)).at(0);
```

### Output

```text
1 ACCRA
```

## Notes

- Unknown field names throw.
- `fields(keys)` is projection, not a hidden accessor binder.

## See Also

- [`Registry::fieldaxis`](fieldaxis.md): project by field-name pattern or slot kind.
- [`Registry::set`](set.md): reinsert a transformed slot into a full registry.

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
