<!-- pluss-hand-authored -->
# Note::replace

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::replace(std::string_view from, std::string_view to) const
```

## Substitute Every Occurrence Of A Literal Substring

`replace(from, to)` returns a new `Note`, same shape as the input, with
**every** occurrence of the literal string `from` in each cell rewritten to
`to`. Despite the bare name, this is a replace-**all**: it is the workhorse
for normalizing separators, renaming a token throughout a batch, or swapping
placeholder text.

The match is **literal** — `from` is compared byte for byte, no regex
metacharacters, no case folding. Occurrences are rewritten **left to right
and non-overlapping**, and the scan resumes *after* the inserted `to` (not
inside it), so a `to` that contains `from` does not cause a second pass:
`replace("aa", "b")` on `"aaaa"` gives `"bb"`.

### Edge cases

- An **empty `from`** is a no-op — the cell is returned unchanged (not an
  error, and not "insert `to` between every character").
- An empty `to` deletes every `from`; that is exactly
  [`remove_all(from)`](remove_all.md).
- No occurrence of `from` leaves the cell unchanged.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. For "first only" or "last only" or "first N", use the sibling
methods `replace_first`, `replace_last`, and `replace_n`.

## Example 1: Every Occurrence Is Rewritten

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"score: 75 score: x", "pending"}).replace("score", "mark"))
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["mark: 75 mark: x", "pending"]
```

Both `score`s in the first cell change; the second cell has none and is
unchanged.

## Example 2: Normalize A Separator

```cpp
const pluss::Note n =
    pluss::make_note({"Ada_Lovelace", "Grace_Hopper"}).replace("_", " ");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Ada Lovelace", "Grace Hopper"]
```

## Example 3: Matches Do Not Overlap And The Result Is Not Re-Scanned

```cpp
pluss::note::print(std::cout, pluss::make_note({"aaaa"}).replace("aa", "b"))
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["bb"]
```

## Example 4: An Empty `from` Is A No-Op

```cpp
std::cout << pluss::note::print(pluss::make_note({"unchanged"}).replace("", "X"))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["unchanged"]
```

## Example 5: An Empty `to` Deletes — Same As `remove_all`

```cpp
std::cout << pluss::note::print(pluss::make_note({"a.b.c"}).replace(".", ""))
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["abc"]
```

## Example 6: Shape Is Preserved

```cpp
std::cout << pluss::note::print(
    pluss::make_note({{"x1", "x2"}, {"x3", "x4"}}).replace("x", "col"))
    << "\n";
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["col1", "col2"], 
  ["col3", "col4"]
]
```

## See Also

- [`Note::remove_all`](remove_all.md): `replace(from, "")` — delete every
  occurrence.
- `pluss::coden::Note::replace_first` / `replace_last` / `replace_n`:
  bounded substitution instead of replace-all.
- [`NoteRegexAccessor::findall`](../note-regex/findall.md): when the target is a pattern,
  not a fixed string.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="repeat_interleave.html">
    <span class="label">Previous</span>
    <span class="title">repeat_interleave</span>
  </a>
  <a class="next-link" href="replace_first.html">
    <span class="label">Next</span>
    <span class="title">replace_first</span>
  </a>
</div>
