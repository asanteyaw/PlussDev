<!-- pluss-hand-authored -->
# Note::TexAccessor::int_norm

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::int_norm() const
```

## Collapse Multi-Integral Commands To `\int`

`int_norm()` rewrites the double/triple/quadruple and contour
integral commands — `\iint`, `\iiint`, `\iiiint`, `\oint`, `\oiint`, and
`\oiiint` — to the plain `\int`. Use it to normalize integral notation
before counting, diffing, or otherwise comparing math source that mixes
these spellings; a plain `\int` is already left as-is.

The match requires a word boundary immediately after the command name.
**A subscript written with `_` does not create that boundary**, because `_`
counts as a word character in the underlying regex engine: `\oint_C` and
`\iint_D` are **not** rewritten, while `\oint\,` or `\oint ` (a following
space or any other non-word character) are. This is the sharpest edge case
of the method — contour and multi-integrals with an attached region
subscript, a very common real notation, pass through unchanged.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: Every Recognized Spelling

```cpp
std::cout << pluss::make_note(
    {"\\iint f \\, dA, \\oint_C, \\iiiint, \\oiint, \\oiiint"})
    .tex().int_norm()
    << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\\int f \\, dA, \\oint_C, \\int, \\int, \\int"]
```

`\iint`, `\iiiint`, `\oiint`, and `\oiiint` all become `\int`. `\oint_C`
does **not**, because the `_C` subscript attaches directly with no word
boundary between `t` and `_` — see the note above.

## Example 2: A Plain `\int` Is Already Canonical

```cpp
std::cout << pluss::make_note({"\\int_0^1 x\\,dx"}).tex().int_norm()
          << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\\int_0^1 x\\,dx"]
```

## Example 3: A Shape-Preserving Batch

```cpp
std::cout << pluss::make_note({"\\iint_D f\\,dA", "\\int already"})
              .tex().int_norm()
          << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["\\iint_D f\\,dA", "\\int already"]
```

Both cells are unchanged: the first because `_D` blocks the boundary match
exactly as in Example 1, the second because it is already `\int`.

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().int_norm().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().int_norm();
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

- `limit_norm`: strip `\limits`/`\nolimits`, which commonly follow
  an integral or summation command.
- `sum_norm`: the equivalent collapse for `\sum` spelling
  variants.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="inline_math.html"><span class="label">Previous</span><span class="title">inline_math</span></a>
  <a class="next-link" href="labels.html"><span class="label">Next</span><span class="title">labels</span></a>
</div>
