<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7d28bb7ae2a9 -->

# RegistryNoteAccessor::replace

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signatures

```cpp
Registry RegistryNoteAccessor::replace(std::string_view from, std::string_view to) const

Registry RegistryNoteAccessor::replace(std::string_view from,
                                       std::string_view to,
                                       const pluss::table::TextOptions& options) const
```

## What It Does

Replaces occurrences of a substring or regex pattern within each bound
field's text, returning a new registry with those fields replaced.
`replace(from, to)` and `replace(from, to, {})` are byte-identical — the
two-argument overload is a fast path to the same underlying primitive with a
default-constructed `TextOptions`.

## Parameters

**from** (`std::string_view`)
    The literal text (or, with `options.regex = true`, the regular
    expression) to search for.

**to** (`std::string_view`)
    The replacement text. With `options.regex = true`, may contain `$1`,
    `$2`, ... backreferences to capture groups in `from`.

**options** (`pluss::table::TextOptions`, three-argument overload only)
    `options.regex` (default `false`): treat `from` as a regex instead of
    literal text. `options.case_sensitive` (default `true`): when `false`,
    matching ignores case, but `to` is still inserted exactly as given.
    `options.limit` (default `-1`, unlimited): caps replacements per value;
    matches never overlap.

## Returns

`Registry`
    A new registry with every bound field's text replaced; every unbound
    field unchanged.

## Examples

Example 1. Default: literal, case-sensitive, unlimited substring replacement.

```cpp
auto rg = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("banana")})},
});

auto out = rg.fields({"city"}).nt().replace("a", "o");
std::cout << std::get<pluss::Note>(out.fields("city").at(0)).at(0);
```

### Output

```text
bonono
```

Every `"a"` is replaced, since matches never overlap and `replace()` keeps
scanning past each replacement.

Example 2. `case_sensitive = false` matches regardless of case, but `to` is
still inserted exactly as written.

```cpp
auto rg = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("BANANA")})},
});

pluss::table::TextOptions opts;
opts.case_sensitive = false;
auto out = rg.fields({"city"}).nt().replace("A", "o", opts);
std::cout << std::get<pluss::Note>(out.fields("city").at(0)).at(0);
```

### Output

```text
BoNoNo
```

Only the matched `"A"`/`"a"` characters are replaced with the lowercase `"o"`
given in `to` — the surrounding letters keep their original case.

## Notes

- `replace(from, to)` and `replace(from, to, {})` are byte-identical.
- Matches never overlap: once a match is replaced, scanning resumes right
  after it.
- With `options.regex = true`, `to` may reference capture groups from `from`
  via `$1`, `$2`, and so on.
- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`RegistryNoteAccessor::has`](has.md): test for a substring/pattern without
  changing the text.
- [`RegistryNoteAccessor::count`](count.md): count matches instead of
  replacing them.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pad.html">
    <span class="label">Previous</span>
    <span class="title">pad</span>
  </a>
  <a class="next-link" href="rstrip.html">
    <span class="label">Next</span>
    <span class="title">rstrip</span>
  </a>
</div>
