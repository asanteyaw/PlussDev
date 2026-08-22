<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 69da50e074c9 -->

# AxelNoteAccessor::strip

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signatures

```cpp
Axel AxelNoteAccessor::strip() const

Axel AxelNoteAccessor::strip(std::string_view chars) const
```

## What It Does

Trims characters from **both** edges of each bound field's text, in
every registry frame, returning a new axel with those fields replaced.
The no-argument overload trims whitespace; the `chars`-argument
overload trims any of the given characters, in any order, from either
end. Applies
[`RegistryNoteAccessor::strip`](../registry-note/strip.md) to every
frame via [`Axel::map`](../axel/map.md).

```{important} AxelNoteAccessor requires every bound field, in every frame, to be Note-kind
Every method on this accessor — not just `strip()` — routes through
`Registry::nt()` per frame, which itself routes through `Slot::nt()`
per field. If any bound field in any frame isn't `Note`-kind (`Series`,
`Timeline`, `TimeIndex`, `Mixed`, `Null`, or `Empty`), the call throws
`"Slot::nt: slot kind must be Note"`. Since [`Axel::nt()`](../axel/nt.md)
binds *every* field of *every* frame by default, this is the most
common way to trip the throw — project down to just the text fields
first with [`fields(keys)`](../axel/fields.md) before calling `.nt()`.
```

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from both edges. Every leading or
    trailing character that appears anywhere in `chars` is removed,
    stopping at the first character on each side that doesn't.

## Returns

`Axel`
    A new axel with every bound field's text trimmed on both edges, in
    every frame; every unbound field unchanged.

## Examples

Example 1. No-argument overload trims leading and trailing whitespace.

```cpp
auto r0 = pluss::scribble::Registry({{"city", pluss::scribble::Slot({pluss::make_note("  Accra  "), pluss::make_note("  Tema  ")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"city"}).nt().strip();
auto city = out.registry(0).fields("city");
std::cout << "'" << std::get<pluss::Note>(city.at(0)).at(0) << "' '"
          << std::get<pluss::Note>(city.at(1)).at(0) << "'";
```

### Output

```text
'Accra' 'Tema'
```

Example 2. `chars`-argument overload trims a specific character set
from both edges.

```cpp
auto r0 = pluss::scribble::Registry({{"code", pluss::scribble::Slot({pluss::make_note("xxAccraxx")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"code"}).nt().strip("x");
std::cout << std::get<pluss::Note>(out.registry(0).fields("code").at(0)).at(0);
```

### Output

```text
Accra
```

Example 3. Calling any method on this accessor with a non-`Note` field
bound throws — see the important callout above.

```cpp
auto r0 = pluss::scribble::Registry({{"id", pluss::scribble::Slot({pluss::make_series(1)})}});
auto axel = pluss::scribble::Axel({r0});

try {
  auto out = axel.fields({"id"}).nt().strip();
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
  method on `AxelNoteAccessor`, not only `strip()`.
- The `chars`-argument overload treats `chars` as a *set* of characters,
  not a literal prefix/suffix — order within `chars` doesn't matter.
- For a one-sided trim, see [`lstrip()`](lstrip.md) and
  [`rstrip()`](rstrip.md).

## See Also

- [`AxelNoteAccessor::lstrip`](lstrip.md) / [`rstrip()`](rstrip.md): the
  one-sided variants.
- [`AxelNoteAccessor::pad`](pad.md): the inverse operation, adding
  characters instead of removing them.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.
- [`Axel::fields`](../axel/fields.md): project down to text-only fields
  before calling `.nt()`, per the important callout above.

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
