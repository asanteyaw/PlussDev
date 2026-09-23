<!-- pluss-hand-authored -->
# NoteTexAccessor::strip_metadata

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::strip_metadata() const
```

## Remove Front-Matter Declarations

`strip_metadata()` removes complete `\title{...}`, `\author{...}`,
`\date{...}`, and `\thanks{...}` calls, plus the bare `\maketitle` command. It
is intended for source that should retain its document body while discarding
title-page identity and presentation directives. It is narrower than
[`strip_preamble()`](strip_preamble.md): packages, document-class declarations,
and document boundaries remain untouched.

The four argument-taking commands are removed only when their required braced
body is complete. Bodies may contain nested balanced braces. Command matching
is exact and case-sensitive; escaped spellings, longer names such as
`\titlecase`, starred forms, and malformed calls survive. `\maketitle` is a
complete bare command and therefore needs no group. Whitespace around removed
regions is collapsed to one ASCII space and each cell is trimmed.

The method allocates a new Text Note and preserves rank, shape, and lane count.
An all-metadata cell becomes an empty string. It is idempotent, propagates
undefined state, and rejects defined non-Text input. The lexical scan is linear
in source bytes and uses stack depth proportional to nested group depth.

## Example 1: Complete Metadata And Nested Bodies

```cpp
const auto source = pluss::make_note({
    R"(A \title{Nested {title}} \author {Ada} \maketitle B)",
    R"(\date{2026} x \thanks{Supported by {Lab}} y)"});
std::cout << source.tex().strip_metadata() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["A B", "x y"]
```

## Example 2: Escaped, Longer, And Incomplete Forms Survive

```cpp
const auto source = pluss::make_note(
    R"(\\date{x} \titlecase{y} \thanks{broken})");
std::cout << source.tex().strip_metadata() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"\\\\date{x} \\titlecase{y} \\thanks{broken"
```

## Example 3: Empty Results Retain Their Matrix Position

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\title{A})", "plain",
                             R"(x \author{B} y)", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().strip_metadata() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["", "plain"],
  ["x y", ""]
]
```

## Example 4: Idempotence And Type Boundaries

```cpp
const auto once = pluss::make_note(R"(A \title{x} B)")
                      .tex().strip_metadata();
std::cout << std::boolalpha
          << "idempotent="
          << ((once == once.tex().strip_metadata()).all().bool_item())
          << " undefined="
          << pluss::Note::undefined().tex().strip_metadata().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_metadata();
} catch (const pluss::Error&) {
  std::cout << "Int64: rejected\n";
}
```

### Output

```text
idempotent=true undefined=false
Int64: rejected
```

## See Also

- [`strip_preamble`](strip_preamble.md): retain only a complete document body.
- [`strip_comments`](strip_comments.md): remove line comments before extraction.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_macro.html"><span class="label">Previous</span><span class="title">strip_macro</span></a>
  <a class="next-link" href="strip_pkgs.html"><span class="label">Next</span><span class="title">strip_pkgs</span></a>
</div>
