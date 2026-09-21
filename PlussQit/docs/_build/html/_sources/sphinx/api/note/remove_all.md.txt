<!-- pluss-hand-authored -->
# Note::remove_all

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::remove_all(std::string_view needle) const
```

## Delete Every Occurrence Of A Literal Substring

`remove_all(needle)` returns a new `Note`, same shape as the input, with
every occurrence of the literal string `needle` deleted from each cell. It is
[`replace`](replace.md) with an empty replacement, given its own name because
"strip this marker out entirely" is a common enough intent to be worth
spelling directly: separators, bracket noise, a repeated tag, a prefix that
leaked into the data.

The match is **literal** — `needle` is compared byte for byte, with no
regex metacharacters and no case folding. Occurrences are removed
**left to right and non-overlapping**: after one match is cut, scanning
resumes at the following byte, so `remove_all("aa")` on `"aaaaa"` removes two
pairs and leaves one `a`. Bytes around the deletions are **not** otherwise
touched — no whitespace collapsing, no trimming — so removing a mid-string
token can leave a double space; chain [`strip`](strip.md) or
[`replace`](replace.md) if that matters.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. A cell that does not contain `needle` is copied through unchanged.

## Example 1: Remove A Repeated Word

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"alpha beta beta", "beta only", "none"}).remove_all("beta"))
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["alpha  ", " only", "none"]
```

Both `beta`s go from the first cell (leaving two spaces where they were), the
leading `beta` goes from the second, and the third is unchanged.

## Example 2: Matches Do Not Overlap

```cpp
const pluss::Note n = pluss::make_note({"aaaaa"}).remove_all("aa");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["a"]
```

`aa` is removed at positions 0 and 2; the trailing `a` has no pair.

## Example 3: A Needle That Is Not Present

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({"keep me"}).remove_all("zzz"))
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["keep me"]
```

## Example 4: Shape Is Preserved

```cpp
std::cout << pluss::note::print(
    pluss::make_note({{"a-b-c", "d-e"}}).remove_all("-"))
    << "\n";
```

### Output

```text
Note(shape=[1, 2], dim=2, numel=2, encoding=utf8)
[
  ["abc", "de"]
]
```

## See Also

- [`Note::replace`](replace.md): the general form — substitute a
  replacement instead of deleting.
- `pluss::coden::Note::remove`: delete only the **first** occurrence.
- [`Note::strip`](strip.md): remove characters only at the cell boundaries.
- [`NoteNlpAccessor::remove_urls`](../note-nlp/remove_urls.md): pattern-based removal (URLs)
  with whitespace cleanup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove.html">
    <span class="label">Previous</span>
    <span class="title">remove</span>
  </a>
  <a class="next-link" href="remove_control_chars.html">
    <span class="label">Next</span>
    <span class="title">remove_control_chars</span>
  </a>
</div>
