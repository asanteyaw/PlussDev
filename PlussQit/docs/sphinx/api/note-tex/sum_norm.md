<!-- pluss-hand-authored -->
# NoteTexAccessor::sum_norm

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::sum_norm() const
```

## Collapse `\sumlimits`/`\sumnolimits` To `\sum`

`sum_norm()` rewrites the literal commands `\sumlimits` and
`\sumnolimits` to the plain `\sum`. Use it to normalize summation notation
before comparing or diffing math source; a plain `\sum` is already left
as-is, and `\sum\limits`/`\sum\nolimits` (the command and the limit-position
modifier written as two separate control words) are not matched here — use
[`limit_norm`](limit_norm.md) for that spelling.

The match is a literal substring replacement, not a word-boundary-aware
regex: `\sumlimits` and `\sumnolimits` are matched and replaced wherever
they occur, with no lookahead or lookbehind. Both spellings are replaced in
the same pass, `\sumnolimits` first has no effect on `\sumlimits`
occurrences and vice versa since neither is a substring of the other's
match position in this implementation.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: Both Spellings

```cpp
std::cout << pluss::make_note({"\\sumlimits_{i=1}^n i \\sumnolimits_j j"})
              .tex().sum_norm()
          << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\\sum_{i=1}^n i \\sum_j j"]
```

## Example 2: A Shape-Preserving Batch

```cpp
std::cout << pluss::make_note({"\\sumlimits", "no change"})
              .tex().sum_norm()
          << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["\\sum", "no change"]
```

## Example 3: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().sum_norm().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().sum_norm();
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

- `limit_norm`: strip the separately spelled `\limits`/`\nolimits`
  modifier from `\sum`, `\int`, and similar commands.
- `int_norm`: the equivalent collapse for multi-integral
  command spellings.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="subsubsections.html"><span class="label">Previous</span><span class="title">subsubsections</span></a>
  <a class="next-link" href="symbols.html"><span class="label">Next</span><span class="title">symbols</span></a>
</div>
