<!-- pluss-hand-authored -->
# NoteTexAccessor::limit_norm

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::limit_norm() const
```

## Strip The `\limits`/`\nolimits` Position Modifier

`limit_norm()` removes every occurrence of the literal commands
`\limits` and `\nolimits` — the modifiers that force a following
subscript/superscript to render above/below (`\limits`) or beside
(`\nolimits`) an operator such as `\sum`, `\int`, or `\max` — leaving the
operator and its scripts otherwise unchanged. Use it to normalize source
that only needs to compare or process the underlying math, not its inline
positioning hints.

After both commands are removed, the result passes through the library's
shared whitespace-normalization step: runs of whitespace collapse to a
single space and the cell is trimmed of leading/trailing whitespace. This
means removing `\limits` never leaves a double space or stray leading/
trailing gap behind, even though the command itself is deleted with nothing
put in its place.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: Both Commands, On Different Operators

```cpp
std::cout << pluss::make_note({"\\lim\\limits_{x \\to 0} f(x) \\sum\\nolimits_i x_i"})
              .tex().limit_norm()
          << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\\lim_{x \\to 0} f(x) \\sum_i x_i"]
```

`\lim\limits_{...}` becomes `\lim_{...}`; `\sum\nolimits_i` becomes
`\sum_i`.

## Example 2: Surrounding Whitespace Collapses

```cpp
std::cout << pluss::make_note({"a \\limits  b"}).tex().limit_norm() << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["a b"]
```

`\limits` is deleted and the two runs of surrounding whitespace it leaves
behind collapse to the single space between `a` and `b`.

## Example 3: A Shape-Preserving Batch

```cpp
std::cout << pluss::make_note({"\\max\\nolimits_x f(x)", "no change"})
              .tex().limit_norm()
          << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["\\max_x f(x)", "no change"]
```

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().limit_norm().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().limit_norm();
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

- `sum_norm`: collapses the separately spelled
  `\sumlimits`/`\sumnolimits` single-command form.
- `int_norm`: the equivalent collapse for multi-integral
  command spellings, an operator this method's `\limits`/`\nolimits`
  modifiers commonly follow.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="labels.html"><span class="label">Previous</span><span class="title">labels</span></a>
  <a class="next-link" href="line_norm.html"><span class="label">Next</span><span class="title">line_norm</span></a>
</div>
