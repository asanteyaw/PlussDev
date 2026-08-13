<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 532e53e78e53 -->

# StreamNoteAccessor::count

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signatures

```cpp
Stream StreamNoteAccessor::count(std::string_view pattern) const

Stream StreamNoteAccessor::count(std::string_view pattern, const pluss::table::TextOptions& options) const
```

## What It Does

Counts non-overlapping occurrences of `pattern` in the bound stream's text,
replacing it with an `Int64` `Series` stream.

```{danger} count() defaults to REGEX matching — the opposite of has()
The single-argument overload treats `pattern` as a **regex** by default
(`options.regex` is effectively `true`) — the opposite default from
[`has()`](has.md), which defaults to literal-substring matching. A pattern
containing regex metacharacters (`.`, `*`, `+`, ...) is interpreted as regex
syntax here even with no `options` argument at all. Pass the two-argument
overload with `TextOptions{.regex = false}` for a literal count.
```

## Parameters

**pattern** (`std::string_view`)
    The pattern to count occurrences of — regex by default (single-argument
    overload), or per `options.regex` (two-argument overload).

**options** (`const pluss::table::TextOptions&`, two-argument overload)
    `options.regex`: treat `pattern` as a regex pattern (default behavior of
    the single-argument overload) or literal text. `options.case_sensitive`
    (default `true`): ignore case when `false`.

## Returns

`Stream`
    A new `Int64` `Series` stream of occurrence counts.

## Examples

Example 1. Default single-argument overload counts regex matches.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"banana"}));
auto out = stream.nt().count("an");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 2                                                                                                        
========================================================================================================================
```

`"an"` occurs twice, non-overlapping, in `"banana"` (`b-AN-AN-a`).

## Notes

- See the important callout above — this is the one method in this accessor's
  pattern family that defaults to regex instead of literal matching.
- Changes the stream's kind from `Note` to `Series`.

## See Also

- [`StreamNoteAccessor::has`](has.md): the sibling method, which defaults to
  literal matching — the opposite default.
- [`StreamNoteAccessor::findall`](findall.md): returns every match's text
  instead of just a count.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::count`](../catalog-note/count.md): the whole-catalog
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="casefold.html">
    <span class="label">Previous</span>
    <span class="title">casefold</span>
  </a>
  <a class="next-link" href="encode.html">
    <span class="label">Next</span>
    <span class="title">encode</span>
  </a>
</div>
