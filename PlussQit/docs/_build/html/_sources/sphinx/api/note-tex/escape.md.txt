<!-- pluss-hand-authored -->
# Note::TexAccessor::escape

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::escape() const
```

## Make Every Cell Safe To Drop Into TeX Source

`escape()` returns a new `Note`, same shape as the input, with every
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

This is the opposite direction from [`to_text`](to_text.md), which
*consumes* TeX markup. The exact inverse of `escape()` is `unescape()`;
`to_text()` is lossy here because it also strips commands and separates the
digits it leaves behind.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Output cells are longer than their inputs wherever a special
appeared.

## Example 1: All Ten Specials

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"a_b $x$ 50% cost #1 & more {y} ~ ^ \\"}).tex().escape())
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
    pluss::make_note({"nothing special here"}).tex().escape();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["nothing special here"]
```

## Example 3: A Scalar

```cpp
pluss::note::print(std::cout, pluss::make_note("C_1 & C_2").tex().escape())
    << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"C\\_1 \\& C\\_2"
```

The cell holds `C\_1 \& C\_2`.

## Example 4: `to_text` Is Not A Clean Inverse

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"rate 50% of x_1"}).tex().escape().tex().to_text())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["rate 50 of x 1"]
```

`escape()` produced `rate 50\% of x\_1`; `to_text()` then removed the `\%`
and `\_` as commands and split the remaining tokens. Use `unescape()` for a
lossless round trip.

## See Also

- `Note::TexAccessor::unescape`: the exact inverse of `escape` (reference page
  not yet written).
- [`Note::TexAccessor::to_text`](to_text.md): the other direction — reduce
  authored TeX to readable plain text (lossy).
- [`Note::url_encode`](../note/url_encode.md) /
  [`Note::base64_encode`](../note/base64_encode.md): escaping/encoding for
  other specific consumers, on root `Note` rather than this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="envs.html">
    <span class="label">Previous</span>
    <span class="title">envs</span>
  </a>
  <a class="next-link" href="figures.html">
    <span class="label">Next</span>
    <span class="title">figures</span>
  </a>
</div>
