<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e0dbc407fdbe -->

# SlotNoteAccessor::has

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::has(std::string_view needle,
                           const pluss::table::TextOptions& options = {}) const
```

## What It Does

Checks each value in the bound slot for the presence of `needle` anywhere
within it, replacing each `Note` value with a `Bool` `Series`. It's the
general substring/pattern search on this accessor — where
[`starts()`](starts.md) and [`ends()`](ends.md) only check the two ends,
`has()` looks everywhere, and can be switched between a literal search
and a regex search via `options`.

The default (`{}`) makes a bare `has(needle)` call a **literal,
case-sensitive substring search**.

```{warning} Different from count()'s single-argument default
`has(needle)` with no `options` searches *literally*, while
[`count()`](count.md)'s single-argument overload defaults to regex mode
instead.
```

## Parameters

**needle** (`std::string_view`)
    The text to search for — a literal substring by default, or a
    regular expression when `options.regex` is `true`.

**options** (`pluss::table::TextOptions`, defaults to `{}`)
    `options.regex` (default `false`): treat `needle` as regex.
    `options.case_sensitive` (default `true`): ignore case when `false`.

## Returns

`Slot`
    A new `Series[bool]` slot.

## Examples

Example 1. Default call: literal, case-sensitive substring search.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("accra north")});
auto out = slot.nt().has("Ac");
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

Example 2. The `case_sensitive` toggle finds it regardless of case.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("accra north")});
pluss::table::TextOptions opts;
opts.case_sensitive = false;
auto out = slot.nt().has("AC", opts);
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
true true
```

## Notes

- Default `TextOptions{}` means literal, case-sensitive — see the
  warning above.
- `regex = true` uses *contains* semantics — for a whole-value match use
  [`fullmatch()`](fullmatch.md).
- Changes each value's kind from `Note` to `Series` (`Bool`).
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::starts`](starts.md) / [`ends()`](ends.md): narrower
  checks anchored to one end.
- [`SlotNoteAccessor::count`](count.md): a related pattern-search method
  whose single-argument default is the opposite of `has()`'s.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fullmatch.html">
    <span class="label">Previous</span>
    <span class="title">fullmatch</span>
  </a>
  <a class="next-link" href="indicate.html">
    <span class="label">Next</span>
    <span class="title">indicate</span>
  </a>
</div>
