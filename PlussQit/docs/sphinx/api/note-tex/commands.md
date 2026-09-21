<!-- pluss-hand-authored -->
# NoteTexAccessor::commands

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signatures

```cpp
TokenBatch NoteTexAccessor::commands() const
TokenBatch NoteTexAccessor::commands(bool with_args) const
```

## Extract Complete Control-Word Lexemes

`commands()` locates TeX control words in every Text cell. A control word is a
backslash followed by ASCII letters or `@`; an immediately following `*` is
part of the lexeme. The default form returns the command spelling itself,
including its leading backslash: `\section*` remains `\section*`.

The Boolean overload controls argument capture, not filtering. With
`with_args == false`, it is exactly the default operation. With
`with_args == true`, scanning continues through every immediately attached,
balanced optional group (`[...]`) and required group (`{...}`). ASCII spacing
between the command and those groups is retained. Nested groups are balanced,
so a title such as `{A {nested} title}` is one argument rather than a truncated
match.

Argument capture is lexical. It deliberately does not know a command's arity,
expand macros, validate whether a command exists, or interpret argument
meaning. It captures the consecutive groups that are present. If the next
group is unbalanced, that group is not attached; the control word is still a
valid result. Comments also remain visible to this scanner. Run
`strip_comments()` first when commented-out commands must be excluded.

Control symbols are not control words. In particular, `\\` is a linebreak and
`\$` is an escaped symbol; neither is reported. This prevents text following a
linebreak from being misread as part of a command. Use `command_names()` when
only names without the leading backslash are required, and
`extract_command_args(name)` when the payload of one known command is the
actual result.

The result is a closed `TokenBatch`: `tokens()` is a flat Text Note in source
order, while `offsets()` is an Int64 Note with one cumulative boundary per
input cell plus the initial zero. Duplicate commands remain duplicated.
Undefined input yields undefined tokens and offsets `[0]`; cells without
commands contribute an empty range. Text is required. The receiver is not
mutated, and extraction performs one lexical pass per cell plus storage for
the returned lexemes and offsets.

## Example 1: Default Command Lexemes

```cpp
const auto source = pluss::make_note({
    R"(\section* [Short] {A {nested} title} and \alpha)",
    R"(line break: \\beta; escaped: \$; malformed: \frac{one)",
    "plain text"});
const auto commands = source.tex().commands();

std::cout << commands.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\section*", "\\alpha", "\\frac"]
```

The linebreak and escaped dollar are control symbols, so neither contributes
a token. The malformed fraction still contains the valid control word
`\frac`.

## Example 2: Include Balanced Arguments

```cpp
const auto complete = source.tex().commands(true);
std::cout << complete.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\section* [Short] {A {nested} title}", "\\alpha", "\\frac"]
```

The optional and required groups attached to `\section*` are retained as one
token. The incomplete `{one` group is not captured.

## Example 3: Spell Out The Default

```cpp
std::cout << std::boolalpha
          << (source.tex().commands(false).tokens() == commands.tokens())
                 .all().bool_item()
          << '\n';
```

### Output

```text
true
```

## Example 4: Offsets Preserve Cell Ownership

```cpp
std::cout << "offsets=[";
const auto offsets = complete.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 3, 3]
```

## Example 5: Undefined Input

```cpp
const auto absent = pluss::Note::undefined().tex().commands();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
```

### Output

```text
false offsets=[0]
```

## Example 6: Non-Text Input Rejects

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().commands(true);
} catch (const pluss::Error&) {
  std::cout << "commands Int64: rejected\n";
}
```

### Output

```text
commands Int64: rejected
```

## See Also

- `command_names`: return names without the leading backslash.
- `extract_command_args`: extract the required argument of one named command.
- `strip_comments`: remove comments before lexical extraction.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="command_names.html"><span class="label">Previous</span><span class="title">command_names</span></a>
  <a class="next-link" href="dedup_blank_lines.html"><span class="label">Next</span><span class="title">dedup_blank_lines</span></a>
</div>
