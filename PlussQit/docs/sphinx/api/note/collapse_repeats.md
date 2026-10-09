<!-- pluss-hand-authored -->
# Note::collapse_repeats

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::collapse_repeats() const
```

## Reduce Every Run Of An Identical Character To One

`collapse_repeats()` returns a new `Note`, same shape as the input, with
every run of two or more of the **same** character in each cell reduced to a
single copy. Use it to normalise "stretched" user text — `"woooow"`,
`"!!!!"`, `"----"` — before matching, or as a rough de-duplication pass.

The operation is **byte-level and unconditional**: it does not know or care
whether the repeated character is a letter, a digit, punctuation, a space,
or part of a multi-byte character. So it also squashes real double letters —
`"book"` becomes `"bok"`, `"hello"` becomes `"helo"`. Reach for it only when
that trade-off is acceptable; use [`collapse_whitespace`](collapse_whitespace.md)
or [`collapse_punct`](collapse_punct.md) when you want to collapse just one
class.

`collapse_repeats` is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"woooow!!!", "aaabbbccc", "book"}).collapse_repeats())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["wow!", "abc", "bok"]
```

`"book"` → `"bok"` shows the trade-off: legitimate double letters collapse
too.

## See Also

- [`Note::collapse_punct`](collapse_punct.md): collapse only runs of
  punctuation.
- [`Note::collapse_whitespace`](collapse_whitespace.md) /
  [`Note::collapse_spaces`](collapse_spaces.md): collapse only whitespace.
- [`Note::RegexAccessor::replace`](../note-regex/replace.md): `regex().replace(R"((.)\1+)",
  "$1")` for a Unicode-safe "collapse repeats" with more control.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="collapse_punct.html">
    <span class="label">Previous</span>
    <span class="title">collapse_punct</span>
  </a>
  <a class="next-link" href="collapse_spaces.html">
    <span class="label">Next</span>
    <span class="title">collapse_spaces</span>
  </a>
</div>
