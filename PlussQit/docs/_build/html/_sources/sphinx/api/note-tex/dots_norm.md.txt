<!-- pluss-hand-authored -->
# NoteTexAccessor::dots_norm

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::dots_norm() const
```

## Collapse Ellipsis Commands To `\dots`

`dots_norm()` rewrites the four `amsmath` ellipsis commands —
`\ldots`, `\cdots`, `\dotsc`, and `\dotsb` — to the plain `\dots`. Use it to
normalize ellipsis notation before comparing or diffing math source; a
plain `\dots` is already left as-is.

The four spellings are matched as literal substrings, one command at a time
in the fixed order `\ldots`, `\cdots`, `\dotsc`, `\dotsb`, with no
word-boundary check. Because `\dots` (the replacement) is not itself a
substring of any of the four patterns, an earlier replacement in the pass
never creates a spurious match for a later pattern in the same pass.

Other ellipsis-family commands — `\dotsi`, `\dotsm`, `\vdots`, `\ddots` —
are not recognized and pass through unchanged; only the four names above
are rewritten.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: All Four Spellings In One Cell

```cpp
std::cout << pluss::make_note({"a \\ldots b \\cdots c \\dotsc d \\dotsb e \\dots f"})
              .tex().dots_norm()
          << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["a \\dots b \\dots c \\dots d \\dots e \\dots f"]
```

Every one of the four rewritten spellings becomes `\dots`; the already-plain
`\dots f` at the end is untouched.

## Example 2: An Unrecognized Ellipsis Command Is Untouched

```cpp
std::cout << pluss::make_note({"a\\cdots b", "no dots"}).tex().dots_norm()
          << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a\\dots b", "no dots"]
```

## Example 3: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().dots_norm().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().dots_norm();
} catch (const pluss::Error&) {
  std::cout << "Int64: rejected\n";
}
```

### Output

```text
false
Int64: rejected
```

## See Also

- `prime_norm`: another lexical-shorthand-to-canonical-command
  normalization, for trailing `'` marks instead of ellipses.
- `to_text`: remove broader TeX structure for readable prose, rather than
  canonicalizing a command spelling.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="display_math.html"><span class="label">Previous</span><span class="title">display_math</span></a>
  <a class="next-link" href="env.html"><span class="label">Next</span><span class="title">env</span></a>
</div>
