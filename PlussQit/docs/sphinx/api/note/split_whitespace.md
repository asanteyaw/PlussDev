<!-- pluss-hand-authored -->
# Note::split_whitespace

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
TokenBatch Note::split_whitespace() const
TokenBatch Note::split_whitespace(std::int64_t limit) const
```

## Split Each Cell On Whitespace Runs

`split_whitespace()` breaks each text cell into tokens separated by
whitespace and returns them as a `TokenBatch`. It is usually the first
tokenization step for prose: fast, structural, and free of empty tokens.

Whitespace is the ASCII set space, `\t`, `\n`, `\r`, `\f`, `\v`. **Any run**
of one or more whitespace bytes is a single separator, and leading and
trailing whitespace are ignored, so consecutive spaces never produce empty
tokens and a padded cell does not gain blank ends. This is the key
difference from [`split(" ")`](split.md), which is a literal single-space
field splitter and *does* keep empty fields.

A cell that is empty or all whitespace yields **no tokens** — an empty range
in the batch, not a single empty token.

### `limit`

`limit` caps the number of splits: after `limit` splits the remainder of the
cell (including any interior whitespace) is emitted as one final token.
`split_whitespace(1)` on `"one two three"` gives `"one"`, `"two three"`. A
negative `limit` (the default) means unlimited.

### The `TokenBatch` result

All tokens from all cells are concatenated into one flat rank-`1` token
`Note` (`batch.tokens()`); `batch.offsets()` (length `numel + 1`) marks each
cell's run: cell `r` is `tokens()[offsets()[r] .. offsets()[r+1])`.

## Example 1: Runs Collapse; No Empty Tokens

```cpp
const auto batch =
    pluss::make_note({"alpha   beta\tgamma", "delta epsilon"}).split_whitespace();

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["alpha", "beta", "gamma", "delta", "epsilon"]
offsets: [0, 3, 5]
```

The triple space and the tab are each one separator.

## Example 2: Leading And Trailing Whitespace Is Ignored

```cpp
const auto batch =
    pluss::make_note({"  Ada Lovelace  ", "\tGrace Hopper\t"}).split_whitespace();
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Ada", "Lovelace", "Grace", "Hopper"]
```

No empty token at either end — unlike `split(" ")`.

## Example 3: Capping The Number Of Splits

```cpp
const auto batch =
    pluss::make_note({"one two three four"}).split_whitespace(1);
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["one", "two three four"]
```

## Example 4: All-Whitespace And Empty Cells Yield Nothing

```cpp
const auto batch = pluss::make_note({"   ", ""}).split_whitespace();

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
offsets: [0, 0, 0]
```

Both input cells map to empty ranges.

## See Also

- [`pluss::note::from_words`](../note_factory/from_words.md): the factory
  that splits a **single** string into a 1-D `Note`.
- [`Note::split`](split.md): literal single-delimiter field splitting that
  keeps empty fields.
- [`Note::tokenize_words`](tokenize_words.md): word tokens that also drop
  punctuation, not just whitespace.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split_lines.html">
    <span class="label">Previous</span>
    <span class="title">split_lines</span>
  </a>
  <a class="next-link" href="starts_with.html">
    <span class="label">Next</span>
    <span class="title">starts_with</span>
  </a>
</div>
