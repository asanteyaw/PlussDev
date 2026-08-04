<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4215dc24d842 -->

# Registry::set

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry Registry::set(std::string_view key, const Slot& value) const
Registry Registry::set(std::initializer_list<std::pair<std::string_view, Slot>> items) const
Registry Registry::set(std::initializer_list<std::pair<std::string, Slot>> items) const
```

## What It Does

Returns a new registry with the named field(s) set to the given `Slot` — this is
**upsert** semantics: a key that already exists gets its `Slot` replaced, and a
key that doesn't exist yet gets added as a new field. This is the primary way to
reinsert a field-scoped transform's result back into a full registry, e.g. after
fetching a field via [`fields(key)`](fields.md) and transforming it directly.

## Parameters

**key** (`std::string_view`)
    A single field name to set (single-item overload).

**value** (`const Slot&`)
    The new `Slot` for that field. Must be defined.

**items** (`std::initializer_list<std::pair<std::string_view, Slot>>` / `std::initializer_list<std::pair<std::string, Slot>>`)
    Several `{key, Slot}` pairs to set at once (multi-item overload).

## Returns

`Registry`
    A new registry with the given field(s) set — existing fields replaced,
    unknown field names added.

## Examples

Example 1. Overwrite an existing field.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.set("id", pluss::bundle::Slot({pluss::make_series(99)}));
auto ids = out.fields("id");
std::cout << "[" << std::get<pluss::Series>(ids.at(0)).to_vec<std::int64_t>()[0] << "] nfields=" << out.nfields();
```

### Output

```text
[99] nfields=2
```

Example 2. Setting a name that doesn't exist yet adds it as a new field.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

auto out = rg.set("population", pluss::bundle::Slot({pluss::make_series(100), pluss::make_series(200), pluss::make_series(300)}));
std::cout << "nfields=" << out.nfields() << " fields: ";
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
nfields=2 fields: id population
```

Example 3. The scoped fetch-transform-reinsert idiom this method enables — see
[`map()`](map.md)'s important callout.

```cpp
auto rg = pluss::bundle::Registry({
    {"population", pluss::bundle::Slot({pluss::make_series(100), pluss::make_series(200), pluss::make_series(300)})},
});

auto doubled = rg.fields("population").transform([](const pluss::bundle::Slot::Value& v) -> pluss::bundle::Slot::Value {
  return pluss::make_series(std::get<pluss::Series>(v).to_vec<std::int64_t>()[0] * 2);
});
auto out = rg.set("population", doubled);
auto pop = out.fields("population");
std::cout << std::get<pluss::Series>(pop.at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
200
```

## Notes

- Upsert semantics: existing keys are replaced, unknown keys are appended as new
  fields — never throws for an unknown key, unlike most other field-scoped
  methods (`push()`, `keep()`, `morph()`, `take()`, `concat()`).
- New fields are appended at the end of field order; existing fields keep their
  original position when replaced.
- The `Slot` passed in must be defined — throws `"Registry::set_slot_: slot undefined"`
  otherwise.
- Never mutates the receiver — use [`set_()`](set_.md) for the in-place form.

## See Also

- [`Registry::set_`](set_.md): the in-place counterpart.
- [`Registry::fields`](fields.md): the typical source of the `Slot` you pass in.
- [`Registry::push`](push.md): append one value to an *existing* field's `Slot`,
  rather than replacing the whole field.
- [`Catalog::set`](../catalog/set.md): the equivalent operation on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_txt.html">
    <span class="label">Previous</span>
    <span class="title">save_txt</span>
  </a>
  <a class="next-link" href="set_.html">
    <span class="label">Next</span>
    <span class="title">set_</span>
  </a>
</div>
