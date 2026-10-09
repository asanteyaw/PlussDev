<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_pkgs

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_pkgs() const
```

## Remove Package And Class Declarations

`strip_pkgs()` removes complete `\usepackage`, `\RequirePackage`, and
`\documentclass` calls. These declarations configure compilation rather than
contribute document prose, so the optional settings and required package or
class list are discarded together. Use [`packages()`](packages.md) when the
individual package names should be extracted instead.

Any number of complete balanced optional groups may precede the required
balanced braced group. Nested braces in an option or package list are handled
without truncation. Recognition uses exact control-word names, so commands
such as `\usepackages` and `\RequirePackageWithOptions` are not mistaken for
the supported declarations. Escaped and incomplete calls remain unchanged.

ASCII whitespace is collapsed after deletion. The returned Text Note is a new,
idempotent value with the receiver's rank, shape, and cell order; a declaration-
only cell becomes an empty string. Undefined input propagates and defined
non-Text dtypes reject. The balanced scan is linear in source bytes.

## Example 1: Classes, Packages, Nested Options, And Boundaries

```cpp
const auto source = pluss::make_note({
    R"(A \documentclass[paper={a4}]{article} B)",
    R"(C \usepackage[opts={x,y}]{alpha,beta} D \RequirePackage{gamma} E)",
    R"(\\usepackage{escaped} \usepackages{long} \usepackage{unfinished)"});
std::cout << source.tex().strip_pkgs() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A B", "C D E", "\\\\usepackage{escaped} \\usepackages{long} \\usepackage{unfinished"]
```

## Example 2: Declaration-Only Cells Remain In Shape

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(\usepackage{a})", "plain",
                             R"(x \documentclass{book} y)", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_pkgs();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "idempotent=" << ((clean == clean.tex().strip_pkgs()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["", "plain"], 
  ["x y", ""]
]
shape=[2, 2]
idempotent=true
```

## Example 3: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_pkgs().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_pkgs();
} catch (const pluss::Error&) {
  std::cout << "strip pkgs Int64: rejected\n";
}
```

### Output

```text
false
strip pkgs Int64: rejected
```

## See Also

- [`packages`](packages.md): extract package names with cell ownership.
- [`strip_bib`](strip_bib.md): remove bibliography configuration and lists.
- `strip_preamble`: remove the complete preamble rather than selected declarations.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_metadata.html"><span class="label">Previous</span><span class="title">strip_metadata</span></a>
  <a class="next-link" href="strip_preamble.html"><span class="label">Next</span><span class="title">strip_preamble</span></a>
</div>
