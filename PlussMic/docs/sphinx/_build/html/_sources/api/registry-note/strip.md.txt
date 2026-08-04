<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 474aacbfd522 -->

# RegistryNoteAccessor::strip

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signatures

```cpp
Registry RegistryNoteAccessor::strip() const

Registry RegistryNoteAccessor::strip(std::string_view chars) const
```

## What It Does

Trims characters from **both** edges of each bound field's text, returning a
new registry with those fields replaced. The no-argument overload trims
whitespace; the `chars`-argument overload trims any of the given characters,
in any order, from either end.

```{important} RegistryNoteAccessor requires every bound field to be Note-kind
Every method on this accessor — not just `strip()` — routes through
`Slot::nt()` for each bound field. If any bound field isn't `Note`-kind
(`Series`, `Timeline`, `TimeIndex`, `Mixed`, `Null`, or `Empty`), the call
throws `"Slot::nt: slot kind must be Note"`. Since [`Registry::nt()`](../registry/nt.md)
binds *every* field of the registry by default, this is the most common way
to trip the throw — project down to just the text fields first with
[`fields(keys)`](../registry/fields.md) before calling `.nt()`.
```

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from both edges. Every leading or
    trailing character that appears anywhere in `chars` is removed, stopping
    at the first character on each side that doesn't.

## Returns

`Registry`
    A new registry with every bound field's text trimmed on both edges;
    every unbound field unchanged.

## Examples

Example 1. No-argument overload trims leading and trailing whitespace.

```cpp
auto rg = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("  Accra  "), pluss::make_note("  Tema  ")})},
});

auto out = rg.fields({"city"}).nt().strip();
auto city = out.fields("city");
std::cout << "'" << std::get<pluss::Note>(city.at(0)).at(0) << "' '"
          << std::get<pluss::Note>(city.at(1)).at(0) << "'";
```

### Output

```text
'Accra' 'Tema'
```

Example 2. `chars`-argument overload trims a specific character set from
both edges.

```cpp
auto rg = pluss::bundle::Registry({
    {"code", pluss::bundle::Slot({pluss::make_note("xxAccraxx")})},
});

auto out = rg.fields({"code"}).nt().strip("x");
std::cout << std::get<pluss::Note>(out.fields("code").at(0)).at(0);
```

### Output

```text
Accra
```

Example 3. Calling any method on this accessor with a non-`Note` field bound
throws — see the important callout above.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
});

try {
  auto out = rg.fields({"id"}).nt().strip();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slot::nt: slot kind must be Note
```

## Notes

- See the important callout above — this precondition applies to every
  method on `RegistryNoteAccessor`, not only `strip()`.
- The `chars`-argument overload treats `chars` as a *set* of characters, not
  a literal prefix/suffix — order within `chars` doesn't matter.
- For a one-sided trim, see [`lstrip()`](lstrip.md) and
  [`rstrip()`](rstrip.md).

## See Also

- [`RegistryNoteAccessor::lstrip`](lstrip.md) / [`rstrip()`](rstrip.md): the
  one-sided variants.
- [`RegistryNoteAccessor::pad`](pad.md): the inverse operation, adding
  characters instead of removing them.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.
- [`Registry::fields`](../registry/fields.md): project down to text-only
  fields before calling `.nt()`, per the important callout above.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="starts.html">
    <span class="label">Previous</span>
    <span class="title">starts</span>
  </a>
  <a class="next-link" href="titlecase.html">
    <span class="label">Next</span>
    <span class="title">titlecase</span>
  </a>
</div>
