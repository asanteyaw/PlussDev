<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7b9b9a9d1c51 -->

# RegistryNoteAccessor::fullmatch

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
Registry RegistryNoteAccessor::fullmatch(std::string_view pattern) const
```

## What It Does

Tests whether each bound field's *entire* text matches `pattern` as a regular
expression, replacing each `Note` field with a `Bool` `Series` field.

Unlike [`matches()`](matches.md), there's no `full` toggle here — `fullmatch()`
always anchors the pattern to both ends of the text (`std::regex_match`
semantics), so a partial match anywhere in the middle is not enough.

## Parameters

**pattern** (`std::string_view`)
    A regular expression the entire value must match, start to end.

## Returns

`Registry`
    A new registry with every bound field replaced by a `Bool` `Series` field;
    every unbound field unchanged.

## Examples

Example 1. A code pattern that matches one value fully and not the other.

```cpp
auto rg = pluss::scribble::Registry({
    {"code", pluss::scribble::Slot({pluss::make_note("AB-123"), pluss::make_note("99XY")})},
});

auto out = rg.fields({"code"}).nt().fullmatch("[A-Z]{2}-[0-9]{3}");
auto code = out.fields("code");
std::cout << std::boolalpha
          << std::get<pluss::Series>(code.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(code.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

`"AB-123"` matches the pattern across its entire length; `"99XY"` doesn't
match the shape at all, so it's `false` — not partially `true`, since
`fullmatch()` requires the whole string.

## Notes

- Always anchored to both ends of the value — contrast
  [`matches(pattern)`](matches.md) (unanchored, "contains" semantics by
  default) and [`matches(pattern, true)`](matches.md) (which reaches the same
  full-match behavior via its `full` parameter).
- There's no `TextOptions` overload here — no `case_sensitive` or `limit`
  control; use inline regex flags (e.g. `(?i)`) in `pattern` if
  case-insensitivity is needed.
- Changes each bound field's kind from `Note` to `Series` (`Bool`).
- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`RegistryNoteAccessor::matches`](matches.md): the more flexible sibling,
  toggling between "contains" and "full match" via its `full` parameter.
- [`RegistryNoteAccessor::has`](has.md): substring/pattern presence, not a
  whole-value shape check.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ends.html">
    <span class="label">Previous</span>
    <span class="title">ends</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
