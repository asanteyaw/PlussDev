<!-- pluss-hand-authored -->
# NoteTexAccessor::warnings

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::warnings() const
```

## Extract Compiler Diagnostics Embedded In Source Or Logs

`warnings()` extracts every occurrence of four recognized LaTeX diagnostic
patterns, each as one whole-match token, in source order:

| Pattern | Example match |
| --- | --- |
| `\PackageWarning{pkg}{msg}` | `\PackageWarning{mypkg}{deprecated macro}` |
| `\PackageError{pkg}{msg}{help}` | `\PackageError{mypkg}{bad option}{See the docs.}` |
| the literal text `Undefined control sequence` | `Undefined control sequence` |
| `LaTeX Error:` through the rest of the line | `LaTeX Error: File not found` |

Unlike the semantic extractors that capture only a command's brace body
(such as [`captions`](captions.md) or [`todos`](todos.md)), `warnings()`
captures the **entire matched span**, brace groups and all — the token for
a `\PackageWarning` match is the whole command call, not just its message
argument. This method is intended to run over compiler log text or source
comments containing embedded diagnostics, not authored document prose,
though it scans whatever Text it is given either way.

The `LaTeX Error:` pattern stops at the end of the physical line: text on a
following line, even immediately after, is never included in that token.
The other three patterns have no line boundary of their own — a
`\PackageWarning{...}{...}` or `\PackageError{...}{...}{...}` call may itself
span multiple lines if its brace arguments do.

The closed `TokenBatch` stores every extracted diagnostic in its rank-1
Text `tokens()` Note. Its Int64 `offsets()` has `source.numel() + 1`
entries, so flattened source cell `i` owns `[offsets[i], offsets[i + 1])`.
The receiver is not mutated.

Undefined yields undefined tokens with offsets `[0]`. Empty cells and cells
with no recognized diagnostic produce empty intervals. Only Text is
accepted.

## Example 1: All Four Patterns, Including The Line Boundary

```cpp
const auto source = pluss::make_note({
    R"(\PackageWarning{mypkg}{deprecated macro} body)",
    R"(Undefined control sequence at line 12)",
    R"(LaTeX Error: File not found)"
    "\ntrailing text on the next line is not part of the match",
    "clean, no diagnostics here"});
const auto warnings = source.tex().warnings();

std::cout << warnings.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\PackageWarning{mypkg}{deprecated macro}", "Undefined control sequence", "LaTeX Error: File not found"]
```

The third cell's `LaTeX Error:` token stops exactly at the newline; the
"trailing text on the next line" is not part of the match, even though it
immediately follows in the same cell. The fourth, clean cell contributes
nothing.

## Example 2: Per-Cell Offsets

```cpp
std::cout << "offsets=[";
const auto offsets = warnings.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 1, 2, 3, 3]
```

Each of the first three cells owns exactly one diagnostic; the fourth owns
none.

## Example 3: A `\PackageError` Call, Whole-Match Captured

```cpp
const auto error_batch =
    pluss::make_note({R"(\PackageError{mypkg}{bad option}{See the docs.})"})
        .tex().warnings();
std::cout << error_batch.tokens() << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\\PackageError{mypkg}{bad option}{See the docs.}"]
```

All three brace groups — package name, message, and help text — are part
of the single captured token.

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().warnings().tokens().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().warnings();
} catch (const pluss::Error&) {
  std::cout << "warnings Int64: rejected\n";
}
```

### Output

```text
false
warnings Int64: rejected
```

## See Also

- [`todos`](todos.md): extract authoring notes instead of compiler
  diagnostics.
- [`strip_comments`](strip_comments.md): remove `%`-comments before
  scanning, if comment text should not be mistaken for a diagnostic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="vector_norm.html"><span class="label">Previous</span><span class="title">vector_norm</span></a>
  <a class="next-link" href="wrap_display_math.html"><span class="label">Next</span><span class="title">wrap_display_math</span></a>
</div>
