<!-- pluss-hand-authored -->
# Note::RegexAccessor::findall

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
TokenBatch Note::RegexAccessor::findall(std::string_view pattern,
                                      std::int64_t flags = 0) const
```

## Extract Every Regex Match From Each Cell

`findall(pattern)` finds all non-overlapping matches of `pattern` in
each text cell and returns them as a `TokenBatch`. It is the
**extraction** member of the regex family — reach for it when the goal is to
pull out substrings (numbers, IDs, dates, hashtags, symbol fragments), not to
test for a match ([`Note::contains`](../note/contains.md) for literals) or to
cut the string apart ([`split`](split.md)).

`pattern` is an ECMAScript regular expression. Matching scans left to right
and does not overlap: after a match, the search resumes at its end.

### Capture groups

If `pattern` contains at least one capture group, each result is the text of
**group 1**; otherwise each result is the whole match. So `\d+` yields the
digit run, while `id=(\d+)` yields just the digits after `id=`. Groups beyond
the first are not returned — restructure the pattern so the piece you want is
group 1.

### `flags`

`flags` is a bitmask of match options; `flags <= 0` (the default) is plain
case-sensitive ECMAScript.

| Bit (value) | Effect |
| --- | --- |
| `1 << 0` (1) | case-insensitive matching |
| `1 << 1` (2) | multiline — `^` and `$` also match at line breaks |

### The `TokenBatch` result

The matches from all cells are concatenated into one rank-one Text `Note`
(`batch.tokens()`). `batch.offsets()` is a rank-one Int64 `Note` of length
`numel + 1`; it records the boundaries, so cell `r` owns `tokens.at(k)` for
`k` in `[offsets[r], offsets[r + 1])`. A cell with no match contributes an
empty range (`offsets[r] == offsets[r + 1]`), never a placeholder token.
Both halves retain Note ownership and can outlive the `TokenBatch` handle.
Call `integers()` only when host code needs to traverse the offsets directly.

### Failure

An invalid `pattern` (unbalanced parenthesis, bad escape) throws
`pluss::Error` before any cell is processed.

## Example 1: Digit Runs

```cpp
const auto n = pluss::make_note(
    {"Order 17 ships 2026-08-02", "Ticket 204 at 18:30", "none"});
const auto batch = n.regex().findall(R"(\d+)");

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[7], dim=1, numel=7, encoding=utf8)
["17", "2026", "08", "02", "204", "18", "30"]
offsets: [0, 4, 7, 7]
```

Cell `0` produced four matches (`offsets[0..1]` = `[0, 4)`), cell `1`
produced three (`[4, 7)`), and cell `2` (`"none"`) produced none
(`[7, 7)` — empty).

## Example 2: A Capture Group Selects Part Of The Match

```cpp
const auto batch =
    pluss::make_note({"k=1; v=2", "x=9"}).regex().findall(R"(\w+=(\d))");

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["1", "2", "9"]
offsets: [0, 2, 3]
```

The match is `k=1`, but the returned token is group 1, `1`.

## Example 3: Case-Insensitive Matching

```cpp
const auto batch =
    pluss::make_note({"AbCdE fF"}).regex().findall(R"([a-z]+)", 1);
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["AbCdE", "fF"]
```

With `flags = 1`, `[a-z]+` also matches upper-case letters, so each word is
one match.

## Example 4: The Same Pattern Without Flags

```cpp
const auto batch =
    pluss::make_note({"AbCdE fF"}).regex().findall(R"([a-z]+)");
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["b", "d", "f"]
```

Case-sensitive, so only the lower-case runs match.

## Example 5: A Cell With No Match

```cpp
const auto batch =
    pluss::make_note({"abc", "123"}).regex().findall(R"(\d+)");

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["123"]
offsets: [0, 0, 1]
```

Cell `0` contributes the empty range `[0, 0)`; cell `1` contributes `[0, 1)`.

## Example 6: An Invalid Pattern Is Rejected

```cpp
try {
  pluss::make_note({"x"}).regex().findall("(");
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
regex().findall: The expression contained mismatched ( and ).
```

## See Also

- [`Note::RegexAccessor::split`](split.md): cut each cell at the matches instead of
  collecting them.
- [`Note::contains`](../note/contains.md): a literal yes/no test, no regex engine.
- [`Note::NlpAccessor::tokens`](../note-nlp/tokens.md) /
  [`Note::split`](../note/split.md): structural tokenizers that also return a
  `TokenBatch`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="match.html">
    <span class="label">Next</span>
    <span class="title">match</span>
  </a>
</div>
