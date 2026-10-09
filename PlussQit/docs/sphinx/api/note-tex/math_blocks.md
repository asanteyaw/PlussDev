<!-- pluss-hand-authored -->
# Note::TexAccessor::math_blocks

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::math_blocks(std::int64_t max_count) const
```

## Extract One Source-Ordered Stream Of Math Regions

`math_blocks()` extracts complete math regions written with any of TeX's four
common delimiter pairs: `$...$`, `$$...$$`, `\(...\)`, and `\[...\]`. Every
token retains its original delimiters, and inline and display regions remain
interleaved in source order. This is the broad extractor to choose when the
distinction between inline and display layout should not split the inventory.

`max_count` controls each receiver cell independently. `-1` means unlimited,
`0` returns an empty range for every cell without scanning for matches, and a
positive value retains at most that many first regions from each cell. Values
below `-1` are invalid and are rejected before extraction.

Single-dollar regions cannot cross a physical newline. Escaped dollars do not
open or close a region, and `$$` is recognized before `$` so a display fragment
is never split into empty inline fragments. Backslash-delimited forms close at
the next unescaped matching delimiter. An unmatched opener contributes no
token. The scan is lexical: it neither executes TeX nor interprets comments,
macro expansion, or environment semantics.

The returned `TokenBatch` stores flat Text `tokens()` plus Int64 `offsets()`.
For receiver cell `i`, `[offsets[i], offsets[i + 1])` is its source-ordered
range. Limiting one cell never consumes another cell's allowance. The flat
token Note therefore remains compact even when a higher-rank receiver has very
uneven match counts. Undefined input returns undefined tokens and offsets
`[0]`; defined non-Text input is rejected.

Without a zero cap, runtime is linear in the visited source bytes plus copied
token bytes. A positive cap permits the scanner to stop matching a lane as soon
as that lane reaches its allowance. Returned strings are owned by the result.

## Option Contract

| `max_count` | Meaning | Scope |
|---|---|---|
| `-1` | Retain every complete region | Per cell |
| `0` | Retain no regions | Per cell |
| Positive | Retain the first `max_count` regions | Per cell |
| Below `-1` | Reject the call | Whole call |

## Example 1: Extract Every Delimiter Form

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(price \$5; $x+\$y$ then $$display$$ and \(z\))",
        "\\[block\\] and $line\nbreak$", R"(\(unfinished)", "plain"},
    std::array<std::int64_t, 2>{2, 2});
const auto all = source.tex().math_blocks(-1);

std::cout << all.tokens() << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["$x+\\$y$", "$$display$$", "\\(z\\)", "\\[block\\]"]
```

The escaped dollar remains inside its inline region. The newline-spanning
single-dollar candidate and unmatched parenthesized opener are absent.

## Example 2: Recover Higher-Rank Lane Ownership

```cpp
std::cout << "offsets=[";
const auto offsets = all.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 3, 4, 4, 4]
```

The first matrix cell owns three regions, the second owns one, and the final
two own none.

## Example 3: Apply A Per-Cell Limit

```cpp
const auto first = source.tex().math_blocks(1);
std::cout << first.tokens() << '\n';

std::cout << "offsets=[";
const auto offsets = first.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["$x+\\$y$", "\\[block\\]"]
offsets=[0, 1, 2, 2, 2]
```

Each of the first two cells receives its own allowance of one token.

## Example 4: Request No Regions

```cpp
const auto none = source.tex().math_blocks(0);
std::cout << none.tokens() << '\n';

std::cout << "offsets=[";
const auto offsets = none.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
offsets=[0, 0, 0, 0, 0]
```

An empty request still retains one boundary for every source lane plus the
final boundary.

## Example 5: Reject An Invalid Limit

```cpp
try {
  (void)source.tex().math_blocks(-2);
} catch (const pluss::Error&) {
  std::cout << "math_blocks(-2): rejected\n";
}
```

### Output

```text
math_blocks(-2): rejected
```

## Example 6: Undefined And Invalid Input

```cpp
const auto absent = pluss::Note::undefined().tex().math_blocks(-1);
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().math_blocks(-1);
} catch (const pluss::Error&) {
  std::cout << "math blocks Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
math blocks Int64: rejected
```

## See Also

- [`inline_math`](inline_math.md): retain only inline forms.
- [`display_math`](display_math.md): retain only display forms.
- [`balance_math_delims`](balance_math_delims.md): repair unmatched delimiters instead of extracting regions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="macro_use.html"><span class="label">Previous</span><span class="title">macro_use</span></a>
  <a class="next-link" href="math_funcs.html"><span class="label">Next</span><span class="title">math_funcs</span></a>
</div>
