<!-- pluss-hand-authored -->
# NoteRegexAccessor::split

[Back to Note Regex Accessor API Reference](../../note_regex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteRegexAccessor::split(std::string_view pattern,
                                    std::int64_t limit = -1) const
```

## Cut Each Cell At Pattern Matches

`split(pattern)` treats every non-overlapping match as a delimiter and returns
the text between delimiters. This is appropriate when the separator has a
shape rather than one exact spelling: mixed comma/semicolon fields, variable
whitespace, or decorated boundaries. Use `Note::split(delimiter)` when the
delimiter is literal; it avoids regular-expression compilation and makes
metacharacters ordinary text.

The pattern uses ECMAScript regular-expression syntax. Delimiter matches are
consumed and do not appear in the result. Text before the first match and
after the last match is retained. Adjacent matches preserve the empty field
between them, and a cell with no match contributes the complete cell as one
piece.

### The `limit` Contract

`limit` counts splits, not returned pieces.

| Value | Behavior |
|---|---|
| Negative, including the default `-1` | Split at every match |
| `0` | Do not split; emit each complete cell once |
| Positive `n` | Perform at most `n` splits per cell, leaving the remainder intact |

The limit is applied independently to every source cell. With `limit = 1`, a
two-cell Note can therefore produce up to four pieces, two from each cell.

### Shape And Ownership

The result is ragged, so it cannot preserve the receiver's rectangular shape
as one Note. `tokens()` contains every piece in row-major cell order.
`offsets()` is an Int64 Note of length `source.numel() + 1`; source cell `i`
owns token indices in `[offsets[i], offsets[i + 1])`. Both Notes own their
storage and remain valid after the temporary accessor and `TokenBatch` handle
are destroyed.

Text input is required. Bytes, Bool, Int64, Float64, and undefined receivers
are rejected. A malformed pattern is rejected before any cell result is
published.

## Example 1: Unlimited Splitting Preserves Empty Pieces

```cpp
const auto source = pluss::make_note({"A1,b22,,C333", "none"});
const auto pieces = source.regex().split(",");

std::cout << pluss::note::print(pieces.tokens()) << '\n';
std::cout << "offsets=[";
for (std::size_t i = 0; i < pieces.offsets().integers().size(); ++i)
  std::cout << (i == 0 ? "" : ", ") << pieces.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["A1", "b22", "", "C333", "none"]
offsets=[0, 4, 5]
```

The adjacent commas create the empty third piece. The second cell contains no
comma and therefore contributes one unchanged piece.

## Example 2: Limit One Leaves The Remaining Text Intact

```cpp
const auto source = pluss::make_note({"A1,b22,,C333", "none"});
const auto pieces = source.regex().split(",", 1);

std::cout << pluss::note::print(pieces.tokens()) << '\n';
std::cout << "offsets=[";
for (std::size_t i = 0; i < pieces.offsets().integers().size(); ++i)
  std::cout << (i == 0 ? "" : ", ") << pieces.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A1", "b22,,C333", "none"]
offsets=[0, 2, 3]
```

Only the first comma in each cell can split. The unmatched remainder is not
rescanned or discarded.

## Example 3: Zero Disables Splitting

```cpp
const auto pieces =
    pluss::make_note({"a,b", "c;d"}).regex().split(R"([,;])", 0);
std::cout << pluss::note::print(pieces.tokens()) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a,b", "c;d"]
```

`limit = 0` is useful when a configurable pipeline must preserve the same call
shape while temporarily disabling segmentation.

## Example 4: A Pattern Can Recognize Several Delimiters

```cpp
const auto pieces =
    pluss::make_note({"north, south;east"}).regex().split(R"(\s*[,;]\s*)");
std::cout << pluss::note::print(pieces.tokens()) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["north", "south", "east"]
```

Including surrounding whitespace in the delimiter pattern removes it as part
of the split. `split()` does not otherwise trim pieces.

## Example 5: Malformed Patterns Fail Before A Result Escapes

```cpp
try {
  (void)pluss::make_note({"a,b"}).regex().split("(");
} catch (const pluss::Error& error) {
  const std::string message(error.what());
  std::cout << message.substr(0, message.find('\n')) << '\n';
}
```

### Output

```text
regex().split: The expression contained mismatched ( and ).
```

## See Also

- [`NoteRegexAccessor::findall`](findall.md): return the matching spans rather than the text between them.
- [`Note::split`](../note/split.md): split on one literal delimiter.
- [`Note::split_whitespace`](../note/split_whitespace.md): split on whitespace runs without compiling a pattern.
- [`NoteNlpAccessor::tokens`](../note-nlp/tokens.md): identify language-oriented word tokens.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="search.html"><span class="label">Previous</span><span class="title">search</span></a>
  <a class="next-link" href="../../note_regex_accessor_api_reference.html"><span class="label">Next</span><span class="title">Regex accessor</span></a>
</div>
