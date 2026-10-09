<!-- pluss-hand-authored -->
# Note::TexAccessor::footnotes

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::footnotes() const
```

## Extract Footnote Bodies

`footnotes()` extracts the required brace argument of each exact `\footnote`
control word. The command and braces are omitted; the authored body becomes
the token. Both `\footnote{body}` and `\footnote[mark]{body}` are recognized,
with the optional mark deliberately excluded because it controls numbering
rather than note content.

Required arguments are parsed as balanced groups. A body such as
`{outer {nested} note}` is therefore returned intact as `outer {nested} note`.
Whitespace may separate the command, optional group, and required group.
Longer command names such as `\footnoted` do not match, an escaped spelling
does not match, and an unclosed optional or required group contributes no
partial token. This is lexical extraction; macro expansion and TeX rendering
are outside its contract.

Each receiver cell may contain a different number of notes, so the method
returns `TokenBatch`. `tokens()` stores all bodies in lane and source order;
`offsets()` maps them back to cells through half-open ranges. Duplicates and
empty footnote bodies are retained because both are authored structures, not
set members. Undefined input returns undefined tokens with offsets `[0]`.
Defined non-Text Notes are rejected.

Scanning and balanced-group parsing are linear in the input bytes, plus the
bytes copied into extracted bodies.

## Example 1: Plain, Marked, And Nested Footnotes

```cpp
const auto source = pluss::make_note({
    R"(Text\footnote{first} and \footnote[7]{outer {nested} note}.)",
    R"(\footnoted{not a footnote} \\footnote{escaped})",
    R"(\footnote{unfinished)"});
const auto notes = source.tex().footnotes();

std::cout << notes.tokens() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["first", "outer {nested} note"]
```

The optional mark `7` is not content. The command-name prefix, escaped
spelling, and malformed call are all ignored.

## Example 2: Recover Lane Ownership

```cpp
std::cout << "offsets=[";
const auto offsets = notes.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 2, 2]
```

Both extracted bodies belong to cell zero. Cells one and two own empty
ranges, rather than placeholder values.

## Example 3: Undefined And Invalid Input

```cpp
const auto absent = pluss::Note::undefined().tex().footnotes();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().footnotes();
} catch (const pluss::Error&) {
  std::cout << "footnotes Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
footnotes Int64: rejected
```

## See Also

- [`args`](args.md): extract the required argument of any selected control word.
- [`captions`](captions.md): extract another optional-plus-required command family.
- [`to_text`](to_text.md): reduce complete TeX source to readable prose.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fix_linebreaks.html"><span class="label">Previous</span><span class="title">fix_linebreaks</span></a>
  <a class="next-link" href="frac_slash.html"><span class="label">Next</span><span class="title">frac_slash</span></a>
</div>
