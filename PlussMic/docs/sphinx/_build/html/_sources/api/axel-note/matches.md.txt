<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f845c6a11d35 -->

# AxelNoteAccessor::matches

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::matches(std::string_view pattern, bool full = false) const
```

## What It Does

Tests each bound field's text against `pattern` as a regular
expression, in every registry frame, replacing each `Note` field with a
`Bool` `Series` field. Unlike [`has()`](has.md), `pattern` here is
*always* treated as a regex — there's no `TextOptions` overload to
switch to a literal search. Applies
[`RegistryNoteAccessor::matches`](../registry-note/matches.md) to every
frame via [`Axel::map`](../axel/map.md).

`full` controls the anchoring:
- `full = false` (the default): `std::regex_search` semantics — the
  pattern just needs to appear somewhere in the value, unanchored
  (start-anchored patterns like `^...` still work as expected).
- `full = true`: `std::regex_match` semantics — the pattern must match
  the *entire* value, equivalent to calling
  [`fullmatch(pattern)`](fullmatch.md).

## Parameters

**pattern** (`std::string_view`)
    A regular expression to test each value against.

**full** (`bool`, defaults to `false`)
    When `true`, the pattern must match the entire value rather than
    just appear somewhere within it.

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by a
    `Bool` `Series` field; every unbound field unchanged.

## Examples

Example 1. Default (`full = false`): the pattern just needs to appear
somewhere in the value.

```cpp
auto r0 = pluss::bundle::Registry({{"code", pluss::bundle::Slot({pluss::make_note("AB-123"), pluss::make_note("99XY")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"code"}).nt().matches("[0-9]+");
auto code = out.registry(0).fields("code");
std::cout << std::boolalpha
          << std::get<pluss::Series>(code.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(code.at(1)).to_vec<bool>()[0];
```

### Output

```text
true true
```

Both values contain a run of digits somewhere, so both are `true` under
the unanchored default.

Example 2. `full = true` requires the whole value to match — an
anchored digits-only pattern rejects `"AB-123"` (which has letters and
a dash) and `"99XY"` (which has letters).

```cpp
auto r0 = pluss::bundle::Registry({{"code", pluss::bundle::Slot({pluss::make_note("AB-123"), pluss::make_note("99XY")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"code"}).nt().matches("^[0-9]+$", true);
auto code = out.registry(0).fields("code");
std::cout << std::boolalpha
          << std::get<pluss::Series>(code.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(code.at(1)).to_vec<bool>()[0];
```

### Output

```text
false false
```

## Notes

- `pattern` is always a regular expression — there's no literal-text
  mode on this method, unlike [`has()`](has.md).
- `matches(pattern, true)` is equivalent to
  [`fullmatch(pattern)`](fullmatch.md).
- Changes each bound field's kind from `Note` to `Series` (`Bool`).
- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.

## See Also

- [`AxelNoteAccessor::fullmatch`](fullmatch.md): the always-anchored
  form, equivalent to `matches(pattern, true)`.
- [`AxelNoteAccessor::has`](has.md): substring/pattern presence with a
  literal-by-default mode this method doesn't have.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lstrip.html">
    <span class="label">Previous</span>
    <span class="title">lstrip</span>
  </a>
  <a class="next-link" href="normalize.html">
    <span class="label">Next</span>
    <span class="title">normalize</span>
  </a>
</div>
