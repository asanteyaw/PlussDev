<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a9efb26dd845 -->

# SlotNoteAccessor::replace

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signatures

```cpp
Slot SlotNoteAccessor::replace(std::string_view from, std::string_view to) const

Slot SlotNoteAccessor::replace(std::string_view from,
                               std::string_view to,
                               const pluss::table::TextOptions& options) const
```

## What It Does

Replaces occurrences of a substring or regex pattern within each value in
the bound slot, returning a new slot. `replace(from, to)` and
`replace(from, to, {})` are byte-identical.

## Parameters

**from** (`std::string_view`)
    The literal text (or, with `options.regex = true`, the regular
    expression) to search for.

**to** (`std::string_view`)
    The replacement text. With `options.regex = true`, may contain `$1`,
    `$2`, ... backreferences.

**options** (`pluss::table::TextOptions`, three-argument overload only)
    `options.regex` (default `false`), `options.case_sensitive` (default
    `true`), `options.limit` (default `-1`, unlimited).

## Returns

`Slot`
    A new slot, matching text replaced.

## Examples

Example 1. Default: literal, case-sensitive, unlimited substring
replacement.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("banana")});
auto out = slot.nt().replace("a", "o");
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
bonono
```

Example 2. `case_sensitive = false` matches regardless of case, but `to`
is still inserted exactly as written.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("BANANA")});
pluss::table::TextOptions opts;
opts.case_sensitive = false;
auto out = slot.nt().replace("A", "o", opts);
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
BoNoNo
```

## Notes

- `replace(from, to)` and `replace(from, to, {})` are byte-identical.
- Matches never overlap.
- With `options.regex = true`, `to` may reference capture groups.
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::has`](has.md): test for a substring without
  changing the text.
- [`SlotNoteAccessor::count`](count.md): count matches instead of
  replacing them.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="partition.html">
    <span class="label">Previous</span>
    <span class="title">partition</span>
  </a>
  <a class="next-link" href="rstrip.html">
    <span class="label">Next</span>
    <span class="title">rstrip</span>
  </a>
</div>
