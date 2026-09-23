<!-- pluss-hand-authored -->
# NoteTexAccessor::args

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::args(std::string_view cmd) const
```

## Select One Command's Required Arguments

`args()` extracts the first required braced argument from every invocation of
one named TeX control word. It is useful when the command name is known but no
dedicated semantic extractor exists: custom metadata commands, project-local
macros, and ordinary commands such as `caption` can all be queried through the
same operation. Pass either `"caption"` or `R"(\caption)"`; the optional
leading backslash identifies the same control word.

The scanner respects control-word boundaries. Asking for `caption` does not
match `captioned`, and a doubled backslash does not begin a command. Whitespace
between the command and its arguments is permitted. One balanced optional
argument is skipped, then the first balanced `{...}` group is returned without
its outer braces. Nested braces and escaped braces remain in the payload.
Malformed calls, calls without a required group, and incomplete optional
groups contribute no token.

Tokens remain in source order and duplicates are retained. The result is a
closed `TokenBatch`: `tokens()` is flat Text storage, while `offsets()` maps
each logical receiver cell to its half-open token range. Receiver rank does not
change that ragged representation. The receiver is not modified.

`cmd` must be a non-empty TeX control-word name made of letters or `@`, with an
optional final `*`; invalid names throw `pluss::Error`. Undefined input yields
undefined tokens and offsets `[0]`. Non-text Notes are rejected. Scanning is
linear in input bytes plus copied payload bytes.

## Example 1: Optional And Nested Groups

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(\caption[Short]{A {nested} title} \caption{Second})",
        R"(\captioned{not a caption} \\caption{escaped})",
        R"(\caption {Third})", R"(\caption[broken{ignored})"},
    std::array<std::int64_t, 2>{2, 2});
const auto result = source.tex().args("caption");
std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A {nested} title", "Second", "Third"]
```

## Example 2: Recover The Four Input Cells

```cpp
const auto offsets = result.offsets().integers();
std::cout << "offsets=[";
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\nshape=[" << source.sizes()[0] << ", "
          << source.sizes()[1] << "]\n";
```

### Output

```text
offsets=[0, 2, 2, 3, 3]
shape=[2, 2]
```

The second and fourth cells own empty ranges; no placeholder token is created.

## Example 3: Name Validation And Dtype Failure

```cpp
try { (void)source.tex().args(""); }
catch (const pluss::Error&) { std::cout << "args(empty): rejected\n"; }
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().args("caption");
} catch (const pluss::Error&) {
  std::cout << "command arguments Int64: rejected\n";
}
```

### Output

```text
args(empty): rejected
command arguments Int64: rejected
```

## See Also

- [`commands`](commands.md): return control words or complete calls rather than one payload.
- [`captions`](captions.md): use the dedicated caption semantics.
- [`env`](env.md): select complete named environment regions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align_blocks.html"><span class="label">Previous</span><span class="title">align_blocks</span></a>
  <a class="next-link" href="balance_braces.html"><span class="label">Next</span><span class="title">balance_braces</span></a>
</div>
