<!-- pluss-hand-authored -->
# Note::unquote

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::unquote() const
```

## Strip One Matching Pair Of Surrounding Quotes

`unquote()` returns a new `Note`, same shape as the input, with one matching
pair of surrounding quote characters removed from each cell. It is the
inverse of [`quote`](quote.md) and the cleanup step for values that arrive
pre-quoted — a CSV field, a shell argument, a config string.

A cell is unquoted only when **both** its first and last characters are the
same quote character — either `"..."` or `'...'`. Then exactly those two
characters are removed and nothing else: no unescaping, no interior
processing, no repeated stripping. If the ends do not match (only one side
quoted, or the two ends are different quote characters, or the cell is
unquoted already), the cell is returned unchanged.

Because neither `quote()` nor `unquote()` escapes, `quote().unquote()`
round-trips exactly, and `unquote()` on a cell whose content itself contains
quotes strips only the outermost pair — see Example 2.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. `""` (two quote characters) unquotes to the empty string.

## Example 1: Double And Single Quotes

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"\"double\"", "'single'", "bare"}).unquote())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["double", "single", "bare"]
```

`"bare"` has no surrounding quotes and is unchanged.

## Example 2: Only A Matching Pair Is Removed

```cpp
const pluss::Note n = pluss::make_note(
    {"\"unbalanced", "trailing\"", "\"\"", "\"a\"b\""}).unquote();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["\"unbalanced", "trailing\"", "", "a\"b"]
```

Cells 0 and 1 are quoted on one side only and are left as-is. `""` unquotes
to empty. `"a"b"` has matching `"` at both ends, so those two are removed,
leaving the interior `a"b`.

## Example 3: Round-Trip With `quote`

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({"payload", "  spaced  "}).quote().unquote())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["payload", "  spaced  "]
```

The boundary spaces survive because `quote`/`unquote` touch only the outermost
characters.

## Example 4: A Scalar

```cpp
std::cout << pluss::note::print(pluss::make_note("\"solo\"").unquote()) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"solo"
```

## See Also

- [`Note::quote`](quote.md): the inverse — wrap each cell in `"..."`.
- [`Note::strip`](strip.md): `strip("\"'")` removes **runs** of quote
  characters from both ends, not just one matching pair.
- `pluss::coden::Note::remove_prefix` / `remove_suffix`: remove a specific
  known delimiter from one end.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tokenize_words.html">
    <span class="label">Previous</span>
    <span class="title">tokenize_words</span>
  </a>
  <a class="next-link" href="upper.html">
    <span class="label">Next</span>
    <span class="title">upper</span>
  </a>
</div>
