<!-- pluss-hand-authored -->
# Note::TexAccessor::packages

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::packages() const
```

## Inventory Package Dependencies Per Cell

`packages()` extracts package names from complete `\usepackage`,
`\RequirePackage`, and `\RequirePackageWithOptions` declarations. A declaration
that names several comma-separated packages contributes one trimmed token per
package. This makes the result a package inventory rather than an inventory of
raw argument strings.

An optional argument may appear between the command and required package group.
Balanced brackets inside that option are consumed as part of the declaration
and do not become tokens. The required braced group is balanced too. Empty
comma fields are ignored, declaration and package order are stable, and
duplicates are retained because repeated declarations can carry different
options or document-local significance.

Matching uses exact control words. `\usepackages` is not `\usepackage`, a
doubled backslash is an escaped command boundary, and a declaration without a
complete required group contributes nothing. The method does not load files,
resolve package installations, merge options, inspect included documents, or
decide whether a package is available.

The result is ragged: `tokens()` is one flat Text Note and `offsets()` is an
Int64 Note with one boundary per receiver cell plus the final boundary. Cell
`i` owns `[offsets[i], offsets[i + 1])`. Higher-rank receivers are traversed in
row-major lane order. Tokens own their strings. Undefined input produces
undefined tokens with offsets `[0]`; defined non-Text input is rejected.

Scanning is linear in source bytes plus copied package-name bytes. It performs
balanced delimiter scans but does not construct a TeX syntax tree.

## Example 1: Options, Lists, And Declaration Forms

```cpp
const auto source = pluss::make_note({
    R"(\usepackage[margin={1in,2in}]{geometry, xcolor})",
    R"(\RequirePackage {amsmath} \RequirePackageWithOptions{hyperref})",
    R"(\\usepackage{escaped} \usepackages{long} \usepackage{, })"});
const auto result = source.tex().packages();

std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["geometry", "xcolor", "amsmath", "hyperref"]
```

The comma inside the balanced option does not split the package list. The
escaped command, longer control word, and empty list contribute nothing.

## Example 2: Recover Declaration Ownership

```cpp
std::cout << "offsets=[";
const auto offsets = result.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 4, 4]
```

The first two cells each declare two packages; the boundary-only third range
is empty.

## Example 3: Undefined And Invalid Input

```cpp
const auto absent = pluss::Note::undefined().tex().packages();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().packages();
} catch (const pluss::Error&) {
  std::cout << "packages Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
packages Int64: rejected
```

## See Also

- [`includes`](includes.md): inventory source and graphics dependencies.
- [`strip_pkgs`](strip_pkgs.md): remove package declarations from retained source.
- [`args`](args.md): extract required arguments from one caller-selected command.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ops.html"><span class="label">Previous</span><span class="title">ops</span></a>
  <a class="next-link" href="pm_signs.html"><span class="label">Next</span><span class="title">pm_signs</span></a>
</div>
