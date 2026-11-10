<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c9a78697b9e2 -->

# RegistryNoteAccessor::has

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
Registry RegistryNoteAccessor::has(std::string_view needle,
                                   const pluss::table::TextOptions& options = {}) const
```

## What It Does

Checks each bound field's text for the presence of `needle` anywhere within
it, replacing each `Note` field with a `Bool` `Series` field. It's the general
substring/pattern search on this accessor — where [`starts()`](starts.md) and
[`ends()`](ends.md) only check the two ends, `has()` looks everywhere, and can
be switched between a literal search and a regex search via `options`.

`TextOptions` has four fields, though only two matter to `has()`:

### `TextOptions` fields

| Field | Type | Default |
|---|---|---|
| `regex` | `bool` | `false` |
| `case_sensitive` | `bool` | `true` |
| `limit` | `std::int64_t` | `-1` |
| `prefix` | `std::string` | `"part"` |

The default (`{}`) makes a bare `has(needle)` call a **literal,
case-sensitive substring search** — `needle` is matched character-for-character,
not compiled as a pattern.

```{warning} Different from count()'s single-argument default
`has(needle)` with no `options` searches *literally*. This is easy to confuse
with [`count()`](count.md), whose single-argument overload defaults to regex
mode instead — so `has(pattern)` and `count(pattern)` treat the exact same
string differently unless you pass `options` explicitly. If a pattern with
regex metacharacters doesn't seem to be matching under `has()`, this default
is the first thing to check — pass `{.regex = true}` explicitly to search by
pattern instead of literal text.
```

## Parameters

**needle** (`std::string_view`)
    The text to search for — a literal substring by default, or a regular
    expression pattern when `options.regex` is `true`.

**options** (`pluss::table::TextOptions`, defaults to `{}`)
    `options.regex` (default `false`): treat `needle` as a regex pattern
    instead of literal text. `options.case_sensitive` (default `true`): when
    `false`, the search ignores case.

## Returns

`Registry`
    A new registry with every bound field replaced by a `Bool` `Series` field;
    every unbound field unchanged.

## Examples

Example 1. Default call: literal, case-sensitive substring search.

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("accra north")})},
});

auto out = rg.fields({"city"}).nt().has("Ac");
auto city = out.fields("city");
std::cout << std::boolalpha
          << std::get<pluss::Series>(city.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(city.at(1)).host<bool>()[0];
```

### Output

```text
true false
```

Row 1's lowercase `"accra north"` doesn't contain the literal, capitalized
`"Ac"`.

Example 2. The `case_sensitive` toggle finds it regardless of case.

```cpp
auto rg = pluss::scribble::Registry({
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("accra north")})},
});

pluss::table::TextOptions opts;
opts.case_sensitive = false;
auto out = rg.fields({"city"}).nt().has("AC", opts);
auto city = out.fields("city");
std::cout << std::boolalpha
          << std::get<pluss::Series>(city.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(city.at(1)).host<bool>()[0];
```

### Output

```text
true true
```

Example 3. Regex mode: search by pattern instead of literal text — the danger
callout above still applies to `count()`, but `has()` needs `options.regex`
set explicitly to opt in.

```cpp
auto rg = pluss::scribble::Registry({
    {"code", pluss::scribble::Slot({pluss::make_note("AB-123"), pluss::make_note("no digits")})},
});

pluss::table::TextOptions opts;
opts.regex = true;
auto out = rg.fields({"code"}).nt().has(R"(\d+)", opts);
auto code = out.fields("code");
std::cout << std::boolalpha
          << std::get<pluss::Series>(code.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(code.at(1)).host<bool>()[0];
```

### Output

```text
true false
```

## Notes

- Default `TextOptions{}` means literal, case-sensitive — unlike
  [`count()`](count.md), whose single-argument overload defaults to regex
  mode. See the warning above.
- `regex = true` uses *contains* semantics — for a whole-value match use
  [`fullmatch()`](fullmatch.md), and for a start-anchored pattern use
  [`matches()`](matches.md).
- Changes each bound field's kind from `Note` to `Series` (`Bool`).
- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`RegistryNoteAccessor::starts`](starts.md) / [`ends()`](ends.md): narrower,
  options-free checks anchored to one end of the value.
- [`RegistryNoteAccessor::matches`](matches.md) / [`fullmatch()`](fullmatch.md):
  regex-only checks anchored to the start, or to the whole value.
- [`RegistryNoteAccessor::count`](count.md): a related pattern-search method
  whose single-argument default is the opposite of `has()`'s.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fullmatch.html">
    <span class="label">Previous</span>
    <span class="title">fullmatch</span>
  </a>
  <a class="next-link" href="length.html">
    <span class="label">Next</span>
    <span class="title">length</span>
  </a>
</div>
