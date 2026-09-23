<!-- pluss-hand-authored -->
# NoteTexAccessor::deaccent

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::deaccent() const
```

## Remove Authored Accent Marks Without Removing Text

`deaccent()` converts accented text to its unaccented form in two representations:
TeX accent commands are unwrapped to their operand, and accented UTF-8 Latin
characters are transliterated to their base character. It is intended for
search keys, comparison text, and other workflows where accent distinctions
should not separate otherwise equivalent words.

The recognized TeX control-symbol accents are `\'`, `\"`, ``\` ``, `\^`,
`\~`, `\=`, and `\.`. The recognized control-word accents are `\v`, `\u`,
`\H`, `\c`, `\k`, `\b`, `\d`, `\r`, and `\t`. An operand may be braced or a
single following UTF-8 code point; spaces after a control-word accent are
syntax separators and are not retained. A balanced braced operand is retained
in full, so `\t{oo}` becomes `oo` rather than losing one character.

Matching is exact. An ordinary command such as `\text{keep}` is not treated as
the `\t` accent, unknown commands remain unchanged, and a doubled backslash is
an escaped command boundary. This distinction prevents accent cleanup from
silently damaging unrelated TeX source. The operation does not transliterate
general scripts, remove punctuation, or erase arbitrary combining syntax.

The operation is lane-wise, out-of-place, and shape preserving. It allocates a
new Text Note and leaves the receiver unchanged. Empty cells remain empty;
undefined input remains undefined; all defined non-Text dtypes are rejected.
The scanner is linear in source bytes and writes directly into one result
string per cell before packing those strings into the output arena.

## Example 1: TeX Accent Forms And Command Boundaries

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{R"(\'{e} \v S \c{c} \t{oo} \text{keep})",
                             "caf\xC3\xA9 d\xC3\xA9j\xC3\xA0",
                             R"(\\'{e} \unknown{x})", ""},
    std::array<std::int64_t, 2>{2, 2});

std::cout << source.tex().deaccent() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["e S c oo \\text{keep}", "cafe deja"],
  ["\\\\'{e} \\unknown{x}", ""]
]
```

Braced, unbraced, control-symbol, and control-word accents are unwrapped. The
ordinary, escaped, and unknown commands remain byte-for-byte intact.

## Example 2: Shape Is Preserved

```cpp
const auto plain = source.tex().deaccent();
std::cout << "shape=[" << plain.sizes()[0] << ", " << plain.sizes()[1]
          << "] dtype=" << pluss::coden::dtype_name(plain.dtype()) << '\n';
```

### Output

```text
shape=[2, 2] dtype=Text
```

## Example 3: The Receiver Is Not Mutated

```cpp
const auto original = pluss::make_note(R"(\'{e})");
const auto plain = original.tex().deaccent();
std::cout << original << '\n' << plain << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"\\'{e}"
Note(shape=[], dim=0, numel=1, encoding=utf8)
"e"
```

## Example 4: Undefined And Invalid Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().deaccent().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().deaccent();
} catch (const pluss::Error&) {
  std::cout << "deaccent Int64: rejected\n";
}
```

### Output

```text
false
deaccent Int64: rejected
```

## See Also

- [`escape`](escape.md): protect literal characters for authored TeX source.
- [`strip_fmt`](strip_fmt.md): unwrap recognized visual formatting commands.
- [`Note::strip_accents`](../note/strip_accents.md): normalize plain text without TeX command semantics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="commands.html"><span class="label">Previous</span><span class="title">commands</span></a>
  <a class="next-link" href="dedup_blank_lines.html"><span class="label">Next</span><span class="title">dedup_blank_lines</span></a>
</div>
