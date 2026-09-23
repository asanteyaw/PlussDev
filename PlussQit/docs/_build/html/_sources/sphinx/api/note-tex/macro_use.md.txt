<!-- pluss-hand-authored -->
# NoteTexAccessor::macro_use

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::macro_use() const
```

## Build A Per-Cell Control-Word Vocabulary

`macro_use()` extracts the distinct TeX control words encountered in each
Text cell. Tokens retain their leading backslash. A following star is retained
too, so `\section*` and `\section` remain distinguishable authored forms. The
first occurrence determines order, and later occurrences of the same complete
lexeme in that cell are suppressed.

Deduplication is deliberately per cell, not global. If `\beta` appears in two
cells, each cell owns a `\beta` token. That contract lets callers compare lane
vocabularies without reconstructing ownership. Use
[`command_names`](command_names.md) when every occurrence is needed and the
leading backslash/star are not wanted.

This is a lexical vocabulary, not a macro-call graph. Built-in commands,
user-defined commands, declaration words, and command names inside arguments
are all control words and may be reported. The extractor does not establish
whether a command is defined, reachable, or executed. Control symbols such as
`\%` are not control words and are omitted. A doubled backslash is skipped as
an escaped pair rather than misread as a command start.

`tokens()` is a flat Text Note and `offsets()` is an Int64 Note. Cell `i` owns
`[offsets[i], offsets[i + 1])`; receiver shape is represented by row-major lane
order and these boundaries. Undefined input returns undefined tokens with the
canonical offsets `[0]`, while defined non-Text input is rejected.

The scanner is linear in source bytes. It maintains a per-cell hash set for
first-seen deduplication and copies each retained lexeme once. Consequently,
memory is proportional to the number and total bytes of distinct control words
in each cell, not the number of occurrences.

## Example 1: First-Seen Deduplication And Starred Forms

```cpp
const auto source = pluss::make_note({
    R"(\alpha \beta \alpha \\gamma \section* \section)",
    R"(\beta \delta \beta)"});
const auto result = source.tex().macro_use();

std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["\\alpha", "\\beta", "\\section*", "\\section", "\\beta", "\\delta"]
```

The repeated `\alpha` is removed from the first cell, but `\section*` does not
collapse into `\section`. The escaped `\\gamma` is absent. `\beta` appears
again because the second cell has its own vocabulary.

## Example 2: Inspect The Per-Cell Ranges

```cpp
std::cout << "offsets=[";
const auto offsets = result.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 4, 6]
```

The first lane owns four distinct lexemes; the second owns two.

## Example 3: Undefined And Invalid Input

```cpp
const auto absent = pluss::Note::undefined().tex().macro_use();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().macro_use();
} catch (const pluss::Error&) {
  std::cout << "macro use Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
macro use Int64: rejected
```

## See Also

- [`macro_defs`](macro_defs.md): inventory complete local declarations.
- [`command_names`](command_names.md): retain every bare control-word occurrence.
- [`commands`](commands.md): extract command lexemes or complete balanced calls.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="macro_subs.html"><span class="label">Previous</span><span class="title">macro_subs</span></a>
  <a class="next-link" href="math_blocks.html"><span class="label">Next</span><span class="title">math_blocks</span></a>
</div>
