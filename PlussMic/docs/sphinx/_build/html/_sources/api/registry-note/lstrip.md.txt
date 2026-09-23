<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8b96cf04a051 -->

# RegistryNoteAccessor::lstrip

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signatures

```cpp
Registry RegistryNoteAccessor::lstrip() const

Registry RegistryNoteAccessor::lstrip(std::string_view chars) const
```

## What It Does

Trims characters from the **left** edge of each bound field's text, returning
a new registry with those fields replaced. The no-argument overload trims
whitespace; the `chars`-argument overload trims any of the given characters,
in any order, repeated as many times as they appear at the start.

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from the left edge. Every leading
    character that appears anywhere in `chars` is removed, stopping at the
    first character that doesn't.

## Returns

`Registry`
    A new registry with every bound field's text left-trimmed; every unbound
    field unchanged.

## Examples

Example 1. No-argument overload trims leading whitespace.

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("  Accra  ")})},
});

auto out = rg.fields({"city"}).nt().lstrip();
std::cout << "'" << std::get<pluss::Note>(out.fields("city").at(0)).at(0) << "'";
```

### Output

```text
'Accra  '
```

Only the leading spaces are removed — the trailing whitespace is untouched,
unlike [`strip()`](strip.md).

Example 2. `chars`-argument overload trims a specific character set from the
left edge only.

```cpp
auto rg = pluss::scribble::Registry({
    {"code", pluss::scribble::Slot({pluss::make_note("xxAccraxx")})},
});

auto out = rg.fields({"code"}).nt().lstrip("x");
std::cout << std::get<pluss::Note>(out.fields("code").at(0)).at(0);
```

### Output

```text
Accraxx
```

## Notes

- Only trims the left edge — see [`rstrip()`](rstrip.md) for the right edge
  and [`strip()`](strip.md) for both.
- The `chars`-argument overload treats `chars` as a *set* of characters, not a
  literal prefix — order within `chars` doesn't matter.
- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`RegistryNoteAccessor::rstrip`](rstrip.md): the right-edge counterpart.
- [`RegistryNoteAccessor::strip`](strip.md): trims both edges at once.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lower.html">
    <span class="label">Previous</span>
    <span class="title">lower</span>
  </a>
  <a class="next-link" href="matches.html">
    <span class="label">Next</span>
    <span class="title">matches</span>
  </a>
</div>
