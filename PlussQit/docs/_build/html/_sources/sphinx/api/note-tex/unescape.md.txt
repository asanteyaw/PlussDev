<!-- pluss-hand-authored -->
# NoteTexAccessor::unescape

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::unescape() const
```

## Reverse `escape()`'s Ten-Character Table

`unescape()` returns a new `Note`, same shape as the input, with every
TeX-escaped special-character sequence from [`escape`](escape.md)'s table
replaced by the literal character it stands for. It is the exact inverse
of `escape()`: applying the two in sequence, in either order on already
plain or already escaped text respectively, is a lossless round trip.

The ten replacements, in the order applied:

| Input | Output |
| --- | --- |
| `\textbackslash{}` | `\` |
| `\{` | `{` |
| `\}` | `}` |
| `\#` | `#` |
| `\$` | `$` |
| `\%` | `%` |
| `\&` | `&` |
| `\_` | `_` |
| `\^{}` | `^` |
| `\~{}` | `~` |

Each replacement is a literal substring match, not a general TeX-command
unescape: a command that merely resembles one of these — `\%something` with
extra trailing text, for instance — still has its recognized `\%` prefix
replaced, since the match only requires that exact sequence to appear, not
that it stand alone. Every other byte is copied unchanged, so text with none
of the ten sequences is returned as-is.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: All Ten Sequences

```cpp
std::cout << pluss::make_note(
    {R"(a\_b \$x\$ 50\% cost \#1 \& more \{y\} \~{} \^{} \textbackslash{})",
     "nothing special here"})
    .tex().unescape()
    << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a_b $x$ 50% cost #1 & more {y} ~ ^ \\", "nothing special here"]
```

Every escaped sequence in the first cell reverts to its literal character;
the second cell, with none of the ten sequences, passes through unchanged.

## Example 2: A Lossless Round Trip Through `escape()`

```cpp
const auto original = pluss::make_note({"a_b $x$ 50% cost #1 & more {y} ~ ^ \\"});
const auto round_trip = original.tex().escape().tex().unescape();
std::cout << std::boolalpha << (round_trip == original).all().bool_item() << '\n';
```

### Output

```text
true
```

`escape()` then `unescape()` reproduces the exact original text.

## Example 3: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().unescape().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().unescape();
} catch (const pluss::Error&) {
  std::cout << "unescape Int64: rejected\n";
}
```

### Output

```text
false
unescape Int64: rejected
```

## See Also

- [`escape`](escape.md): the exact inverse — protect literal text for TeX.
- [`to_text`](to_text.md): a broader, lossy reduction from authored TeX to
  plain text — unlike `unescape()`, it also strips commands and comments.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="todos.html"><span class="label">Previous</span><span class="title">todos</span></a>
  <a class="next-link" href="vector_norm.html"><span class="label">Next</span><span class="title">vector_norm</span></a>
</div>
