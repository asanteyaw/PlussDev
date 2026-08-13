<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 90375704d97c -->

# StreamNoteAccessor::replace

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signatures

```cpp
Stream StreamNoteAccessor::replace(std::string_view from, std::string_view to) const

Stream StreamNoteAccessor::replace(std::string_view from, std::string_view to, const pluss::table::TextOptions& options) const
```

## What It Does

Replaces every occurrence of `from` with `to` in the bound stream's text,
returning a new stream. The two-argument overload treats `from` as literal
text; the three-argument overload lets `options.regex` switch to a regex
replacement.

## Parameters

**from** (`std::string_view`)
    The text to replace — literal under the two-argument overload, or
    literal/regex under the three-argument overload depending on
    `options.regex`.

**to** (`std::string_view`)
    The replacement text.

**options** (`pluss::table::TextOptions`, three-argument overload only)
    `options.regex` (default `false`): treat `from` as a regex pattern.
    `options.case_sensitive` (default `true`): ignore case when `false`.
    `options.limit` (default `-1`, unlimited): cap the number of replacements
    per value.

## Returns

`Stream`
    A new stream with its text replaced.

## Examples

Example 1. Two-argument overload: every literal `"a"` becomes `"X"`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra", "Kumasi"}));
auto out = stream.nt().replace("a", "X");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | AccrX                                                                                                    
1            | KumXsi                                                                                                   
========================================================================================================================
```

`"Accra"`'s capital `"A"` is untouched (case-sensitive by default) — only the
lowercase `"a"` at the end is replaced. `"Kumasi"`'s `"a"` becomes `"X"` too.

## Notes

- The two-argument overload's `from` is always literal — matching
  [`has()`](has.md)'s two-argument-overload default, not
  [`count()`](count.md)'s single-argument-overload default.
- Case-sensitive by default; pass `TextOptions{.case_sensitive = false}` via
  the three-argument overload for case-insensitive replacement.

## See Also

- [`StreamNoteAccessor::count`](count.md) / [`has()`](has.md): related pattern
  operations sharing the same `TextOptions` type.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::replace`](../catalog-note/replace.md): the
  whole-catalog equivalent.

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
