<!-- pluss-hand-authored -->
# Note::TexAccessor::command_names

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::command_names() const
```

## Extract TeX Control-Word Names

`command_names()` scans each cell for TeX control words and returns their
names without the leading backslash. A control word begins with `\` followed
by ASCII letters or `@`; the letter/`@` run is the name, and an immediately
following `*` is retained. Thus `\alpha`, `\section*`, and `\foo@bar` yield
`alpha`, `section*`, and `foo@bar`.

Control symbols are a different TeX form and are deliberately absent. Escaped
specials such as `\$` and `\%` do not have word names. The linebreak command
`\\` is consumed as a control symbol, so letters immediately following it are
not falsely reported as a command. This distinction matters when extracting
features from mixed prose and layout source.

The method reports lexical occurrences, not a set. Names retain source order
and duplicates remain duplicated. It does not determine whether a command is
defined, distinguish built-ins from user macros, expand declarations, or
discard comments. Apply `strip_comments()` first when commented source must
not contribute names; use `macro_use()` when a deduplicated macro-oriented
inventory is the intended result.

The return is a closed `TokenBatch`. Flat Text `tokens()` holds the names and
Int64 `offsets()` maps every flattened source cell to a half-open token range.
Undefined yields undefined tokens with offsets `[0]`; empty/plain-text cells
produce empty ranges. Text is required, and extraction allocates only the
matched names and offsets after one linear scan of the source bytes.

## Example 1: Words, Stars, Internal Names, Symbols, And Linebreaks

```cpp
const auto source = pluss::make_note({
    R"(\alpha + \section*{Intro} + \foo@bar)",
    R"(line break: \\alpha; symbols: \$ and \%; then \beta)",
    "plain text"});
const auto names = source.tex().command_names();

std::cout << names.tokens() << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["alpha", "section*", "foo@bar", "beta"]
```

`alpha` after the linebreak `\\` is plain following text, while the later
`\beta` is a control word.

## Example 2: Offsets Preserve Source Cells

```cpp
std::cout << "offsets=[";
const auto offsets = names.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 3, 4, 4]
```

## Example 3: Undefined Produces The Closed Empty Sentinel

```cpp
const auto absent = pluss::Note::undefined().tex().command_names();
std::cout << std::boolalpha << "undefined names="
          << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
```

### Output

```text
undefined names=false offsets=[0]
```

## Example 4: Native Integer Cells Reject

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().command_names();
} catch (const pluss::Error&) {
  std::cout << "command names Int64: rejected\n";
}
```

### Output

```text
command names Int64: rejected
```

## See Also

- `commands`: extract complete command lexemes rather than names alone.
- `macro_defs`: extract names introduced by local declarations.
- `macro_use`: build a deduplicated macro-use inventory.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="collapse_macros.html"><span class="label">Previous</span><span class="title">collapse_macros</span></a>
  <a class="next-link" href="commands.html"><span class="label">Next</span><span class="title">commands</span></a>
</div>
