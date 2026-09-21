# Note::split_chars

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
TokenBatch Note::split_chars() const
```

## Split Every Text Cell Into Unicode Scalars

`split_chars()` tokenizes each Text lane into its decoded Unicode scalar
values. It is useful when an algorithm needs code-point-level traversal while
retaining the boundary of every original cell. The result is a `TokenBatch`:
all scalar strings are concatenated in `tokens()`, and Int64 `offsets()` map
each flattened source lane to its half-open token range.

This is deliberately not a byte splitter and not a user-perceived character
or grapheme splitter. A multibyte UTF-8 scalar such as `é` or `🙂` becomes one
token, while a decomposed `e` followed by a combining acute accent becomes two
tokens. Use [`lengths_graphemes()`](lengths_graphemes.md) when cursor positions
or visible character counts require grapheme-cluster semantics.

For a source with `N = numel()` lanes, `offsets()` has `N + 1` entries. Source
lane `i` owns `tokens()` positions `[offsets[i], offsets[i + 1])`. Empty source
lanes contribute no tokens, so adjacent offsets may be equal. Higher-rank
inputs are traversed in flat row-major order; the TokenBatch retains lane
boundaries rather than the source rank because each lane may produce a
different number of scalars.

Only Text Notes are accepted. Bytes are not implicitly decoded, and Bool,
Int64, and Float64 are not formatted into text. Convert deliberately before
calling when that is the desired operation. An undefined receiver propagates
as an undefined token Note with the canonical offset Note `[0]`.

The operation is non-mutating. It walks the UTF-8 sequence widths already held
by the Text Note, allocates one small Text cell per scalar plus the offsets
Note, and is linear in source bytes. It is not the malformed-input validation
boundary; use [`to_utf8_strict()`](to_utf8_strict.md) before splitting when text
originates from an untrusted byte source. Large Notes are processed by the
batched tokenization path, but output cardinality still determines allocation
cost.

## Example 1: ASCII And Multibyte Scalars

```cpp
const auto batch = pluss::note::make({"Aé🙂", "Hi"}).split_chars();

std::cout << "tokens=" << batch.tokens() << '\n';
std::cout << "offsets=[";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
tokens=Note(shape=[5], dim=1, numel=5, encoding=utf8)
["A", "é", "🙂", "H", "i"]
offsets=[0, 3, 5]
```

The first source cell owns three tokens and the second owns two, despite the
different UTF-8 byte widths involved.

## Example 2: Recover Each Source Lane

```cpp
const auto batch = pluss::note::make({"Aé🙂", "Hi"}).split_chars();
for (std::int64_t row = 0; row < 2; ++row) {
  std::cout << "row " << row << ": [";
  for (auto i = batch.offsets().integers()[row];
       i < batch.offsets().integers()[row + 1]; ++i) {
    std::cout << (i == batch.offsets().integers()[row] ? "\"" : ", \"")
              << batch.tokens().at(i) << '"';
  }
  std::cout << "]\n";
}
```

### Output

```text
row 0: ["A", "é", "🙂"]
row 1: ["H", "i"]
```

## Example 3: Combining Marks Remain Separate Scalars

```cpp
const auto batch = pluss::note::make("e\u0301").split_chars();
std::cout << "decomposed token-count=" << batch.tokens().numel()
          << " byte-lengths=" << batch.tokens().at(0).size()
          << ',' << batch.tokens().at(1).size() << '\n';
```

### Output

```text
decomposed token-count=2 byte-lengths=1,2
```

The two scalars may render as one visual glyph. Normalize first if canonical
composition is part of the intended pipeline.

## Example 4: Matrices Flatten Into Ragged Rows

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{"ab", "", "x", "YZ"},
    std::array<std::int64_t, 2>{2, 2});
const auto batch = grid.split_chars();

std::cout << "grid source-shape=[" << grid.size(0) << ", " << grid.size(1)
          << "] offsets=";
for (const auto offset : batch.offsets().integers()) std::cout << offset << ' ';
std::cout << '\n';
```

### Output

```text
grid source-shape=[2, 2] offsets=0 2 2 3 5 
```

The empty cell is the second flattened lane and therefore contributes the
repeated boundary `2, 2`.

## Example 5: Undefined Propagates Structurally

```cpp
const auto batch = pluss::Note::undefined().split_chars();
std::cout << std::boolalpha
          << "undefined batch=" << batch.defined()
          << " tokens-defined=" << batch.tokens().defined()
          << " offset=" << batch.offsets().int_item() << '\n';
```

### Output

```text
undefined batch=true tokens-defined=false offset=0
```

The batch object is valid and its offsets are canonical, while token absence
remains observable.

## Example 6: Non-Text Dtypes Reject

```cpp
try {
  (void)pluss::note::bytes(std::vector<std::string>{"abc"}).split_chars();
} catch (const pluss::Error&) {
  std::cout << "Bytes split: rejected\n";
}
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).split_chars();
} catch (const pluss::Error&) {
  std::cout << "Int64 split: rejected\n";
}
```

### Output

```text
Bytes split: rejected
Int64 split: rejected
```

## See Also

- [`lengths_chars`](lengths_chars.md): count Unicode scalar values without materializing them.
- [`lengths_graphemes`](lengths_graphemes.md): count user-perceived grapheme clusters.
- [`split`](split.md): split each lane on a literal delimiter.
- [`TokenBatch`](../../tokenbatch_api_reference.md): consume ragged token results.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split.html"><span class="label">Previous</span><span class="title">split</span></a>
  <a class="next-link" href="split_lines.html"><span class="label">Next</span><span class="title">split_lines</span></a>
</div>
<!-- pluss-hand-authored -->
