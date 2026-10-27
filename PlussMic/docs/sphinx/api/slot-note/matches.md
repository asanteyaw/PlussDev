<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c5177ea32eaa -->

# SlotNoteAccessor::matches

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::matches(std::string_view pattern, bool full = false) const
```

## What It Does

Tests each value in the bound slot against `pattern` as a regular
expression, replacing each `Note` value with a `Bool` `Series`. Unlike
[`has()`](has.md), `pattern` here is *always* treated as a regex.

`full` controls the anchoring: `false` (default) uses `regex_search`
(contains semantics); `true` uses `regex_match` (whole-value, equivalent
to [`fullmatch(pattern)`](fullmatch.md)).

## Parameters

**pattern** (`std::string_view`)
    A regular expression to test each value against.

**full** (`bool`, defaults to `false`)
    When `true`, the pattern must match the entire value.

## Returns

`Slot`
    A new `Series[bool]` slot.

## Examples

Example 1. Default (`full = false`): the pattern just needs to appear
somewhere.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("AB-123"), pluss::make_note("99XY")});
auto out = slot.nt().matches("[0-9]+");
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).host<bool>()[0];
```

### Output

```text
true true
```

Example 2. `full = true` requires the whole value to match.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("AB-123"), pluss::make_note("99XY")});
auto out = slot.nt().matches("^[0-9]+$", true);
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).host<bool>()[0];
```

### Output

```text
false false
```

## Notes

- `pattern` is always a regular expression — no literal-text mode.
- `matches(pattern, true)` is equivalent to
  [`fullmatch(pattern)`](fullmatch.md).
- Changes each value's kind from `Note` to `Series` (`Bool`).
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::fullmatch`](fullmatch.md): the always-anchored form.
- [`SlotNoteAccessor::has`](has.md): substring/pattern presence with a
  literal-by-default mode this method doesn't have.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
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
