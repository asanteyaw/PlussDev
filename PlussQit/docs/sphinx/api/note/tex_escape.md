<!-- pluss-hand-authored -->
# Note::tex_escape

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::tex_escape() const
```

## Make Every Cell Safe To Drop Into TeX Source

`tex_escape()` returns a new `Note`, same shape as the input, with every
TeX/LaTeX special character in each cell replaced by its escaped form. Use it
when arbitrary text — a name, a title, a value from a dataset — has to be
inserted into a `.tex` document or a formula label without the TeX compiler
mis-reading a `_`, `%`, or `$`.

The ten characters TeX treats specially are escaped as follows:

| Input | Output |
| --- | --- |
| `\` | `\textbackslash{}` |
| `{` | `\{` |
| `}` | `\}` |
| `#` | `\#` |
| `$` | `\$` |
| `%` | `\%` |
| `&` | `\&` |
| `_` | `\_` |
| `^` | `\^{}` |
| `~` | `\~{}` |

Every other byte is copied unchanged, so text with no specials is returned
as-is. The escape is purely lexical — it does not know or care whether the
cell was already partly escaped, so escaping twice doubles the backslashes.

This is the opposite direction from [`tex_to_text`](tex_to_text.md), which
*consumes* TeX markup. The exact inverse of `tex_escape()` is
`tex_unescape()`; `tex_to_text()` is lossy here because it also strips
commands and separates the digits it leaves behind.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Output cells are longer than their inputs wherever a special
appeared.

## Example 1: All Ten Specials

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"a_b $x$ 50% cost #1 & more {y} ~ ^ \\"}).tex_escape())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["a\\_b \\$x\\$ 50\\% cost \\#1 \\& more \\{y\\} \\~{} \\^{} \\textbackslash{}"]
```

`print` shows each output backslash as `\\`; the cell holds single
backslashes (`a\_b \$x\$ …`).

## Example 2: Plain Text Is Unchanged

```cpp
const pluss::Note n =
    pluss::make_note({"nothing special here"}).tex_escape();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["nothing special here"]
```

## Example 3: A Scalar

```cpp
pluss::note::print(std::cout, pluss::make_note("C_1 & C_2").tex_escape())
    << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"C\\_1 \\& C\\_2"
```

The cell holds `C\_1 \& C\_2`.

## Example 4: `tex_to_text` Is Not A Clean Inverse

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"rate 50% of x_1"}).tex_escape().tex_to_text())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["rate 50 of x 1"]
```

`tex_escape` produced `rate 50\% of x\_1`; `tex_to_text` then removed the
`\%` and `\_` as commands and split the remaining tokens. Use
`tex_unescape()` for a lossless round trip.

## See Also

- `pluss::coden::Note::tex_unescape`: the exact inverse of `tex_escape`.
- [`Note::tex_to_text`](tex_to_text.md): the other direction — reduce
  authored TeX to readable plain text (lossy).
- [`Note::url_encode`](url_encode.md) /
  [`Note::base64_encode`](base64_encode.md): escaping/encoding for other
  specific consumers.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tensor3.html">
    <span class="label">Previous</span>
    <span class="title">tensor3</span>
  </a>
  <a class="next-link" href="tex_to_text.html">
    <span class="label">Next</span>
    <span class="title">tex_to_text</span>
  </a>
</div>
