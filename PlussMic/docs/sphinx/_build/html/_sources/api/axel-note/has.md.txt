<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5eacbb0e84dd -->

# AxelNoteAccessor::has

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::has(std::string_view needle,
                           const pluss::table::TextOptions& options = {}) const
```

## What It Does

Checks each bound field's text for the presence of `needle` anywhere
within it, in every registry frame, replacing each `Note` field with a
`Bool` `Series` field. It's the general substring/pattern search on
this accessor — where [`starts()`](starts.md) and [`ends()`](ends.md)
only check the two ends, `has()` looks everywhere, and can be switched
between a literal search and a regex search via `options`. Applies
[`RegistryNoteAccessor::has`](../registry-note/has.md) to every frame
via [`Axel::map`](../axel/map.md).

`TextOptions` has four fields, though only two matter to `has()`:

```cpp
struct TextOptions {
  bool regex{false};
  bool case_sensitive{true};
  std::int64_t limit{-1};      // unused by has()
  std::string prefix{"part"};  // unused by has()
};
```

The default (`{}`) makes a bare `has(needle)` call a **literal,
case-sensitive substring search** — `needle` is matched
character-for-character, not compiled as a pattern.

```{warning} Different from count()'s single-argument default
`has(needle)` with no `options` searches *literally*. This is easy to
confuse with [`count()`](count.md), whose single-argument overload
defaults to regex mode instead — so `has(pattern)` and `count(pattern)`
treat the exact same string differently unless you pass `options`
explicitly. If a pattern with regex metacharacters doesn't seem to be
matching under `has()`, this default is the first thing to check — pass
`{.regex = true}` explicitly to search by pattern instead of literal
text.
```

## Parameters

**needle** (`std::string_view`)
    The text to search for — a literal substring by default, or a
    regular expression pattern when `options.regex` is `true`.

**options** (`pluss::table::TextOptions`, defaults to `{}`)
    `options.regex` (default `false`): treat `needle` as a regex
    pattern instead of literal text. `options.case_sensitive` (default
    `true`): when `false`, the search ignores case.

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by a
    `Bool` `Series` field; every unbound field unchanged.

## Examples

Example 1. Default call: literal, case-sensitive substring search.

```cpp
auto r0 = pluss::scribble::Registry({{"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("accra north")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"city"}).nt().has("Ac");
auto city = out.registry(0).fields("city");
std::cout << std::boolalpha
          << std::get<pluss::Series>(city.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(city.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

Row 1's lowercase `"accra north"` doesn't contain the literal,
capitalized `"Ac"`.

Example 2. The `case_sensitive` toggle finds it regardless of case.

```cpp
auto r0 = pluss::scribble::Registry({{"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("accra north")})}});
auto axel = pluss::scribble::Axel({r0});

pluss::table::TextOptions opts;
opts.case_sensitive = false;
auto out = axel.fields({"city"}).nt().has("AC", opts);
auto city = out.registry(0).fields("city");
std::cout << std::boolalpha
          << std::get<pluss::Series>(city.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(city.at(1)).to_vec<bool>()[0];
```

### Output

```text
true true
```

Example 3. Regex mode: search by pattern instead of literal text — the
danger callout on [`count()`](count.md) still applies there, but
`has()` needs `options.regex` set explicitly to opt in.

```cpp
auto r0 = pluss::scribble::Registry({{"code", pluss::scribble::Slot({pluss::make_note("AB-123"), pluss::make_note("no digits")})}});
auto axel = pluss::scribble::Axel({r0});

pluss::table::TextOptions opts;
opts.regex = true;
auto out = axel.fields({"code"}).nt().has(R"(\d+)", opts);
auto code = out.registry(0).fields("code");
std::cout << std::boolalpha
          << std::get<pluss::Series>(code.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(code.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

## Notes

- Default `TextOptions{}` means literal, case-sensitive — unlike
  [`count()`](count.md), whose single-argument overload defaults to
  regex mode. See the warning above.
- `regex = true` uses *contains* semantics — for a whole-value match
  use [`fullmatch()`](fullmatch.md), and for the same unanchored search
  by way of a boolean, [`matches()`](matches.md).
- Changes each bound field's kind from `Note` to `Series` (`Bool`).
- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.

## See Also

- [`AxelNoteAccessor::starts`](starts.md) / [`ends()`](ends.md):
  narrower, options-free checks anchored to one end of the value.
- [`AxelNoteAccessor::matches`](matches.md) / [`fullmatch()`](fullmatch.md):
  regex-only checks.
- [`AxelNoteAccessor::count`](count.md): a related pattern-search method
  whose single-argument default is the opposite of `has()`'s.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

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
