<!-- pluss-hand-authored -->
# NoteTexAccessor::simplify_roots

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::simplify_roots() const
```

## Canonicalize Root Syntax While Retaining TeX

`simplify_roots()` normalizes complete square-root and indexed-root calls to
`\sqrt{radicand}` and `\sqrt[index]{radicand}`. It removes whitespace between
the command and its groups and trims only the outer edge of each group. Use it
to obtain stable TeX source for comparison, caching, or later transforms. Use
[`sqrt_pow()`](sqrt_pow.md) instead when roots should become explicit powers.

The command name is exact and case-sensitive. An optional index must be a
balanced bracket group, and the radicand must be a balanced brace group.
Nested braces and nested roots are normalized recursively. Longer names,
escaped spellings, starred forms, missing groups, and unbalanced calls are
preserved rather than partially interpreted. The method does not evaluate a
root or decide whether an index is mathematically valid.

The operation is out-of-place, idempotent, shape preserving, and Text-only.
Undefined state propagates. Runtime is linear in source bytes with recursion
proportional to root nesting depth.

## Example 1: Square, Indexed, And Nested Roots

```cpp
const auto source = pluss::make_note({
    R"(\sqrt [ 3 ] { a + \sqrt { {b} } })",
    R"(\sqrt { x } + \sqrt[ n + 1 ]{ y })"});
std::cout << source.tex().simplify_roots() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["\\sqrt[3]{a + \\sqrt{{b}}}", "\\sqrt{x} + \\sqrt[n + 1]{y}"]
```

## Example 2: Ambiguous Source Is Preserved

```cpp
const auto source = pluss::make_note(
    R"(\sqrted{x} \\sqrt{x} \sqrt[3]oops \sqrt{broken)");
std::cout << source.tex().simplify_roots() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"\\sqrted{x} \\\\sqrt{x} \\sqrt[3]oops \\sqrt{broken"
```

## Example 3: Higher-Rank Shape Is Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\sqrt {a})", "plain",
                             R"(\sqrt [4] {b})", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().simplify_roots() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["\\sqrt{a}", "plain"],
  ["\\sqrt[4]{b}", ""]
]
```

## Example 4: Idempotence And Type Boundaries

```cpp
const auto once = pluss::make_note(R"(\sqrt {x})").tex().simplify_roots();
std::cout << std::boolalpha
          << "idempotent="
          << ((once == once.tex().simplify_roots()).all().bool_item())
          << " undefined="
          << pluss::Note::undefined().tex().simplify_roots().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().simplify_roots();
} catch (const pluss::Error&) {
  std::cout << "Int64: rejected\n";
}
```

### Output

```text
idempotent=true undefined=false
Int64: rejected
```

## See Also

- [`sqrt_pow`](sqrt_pow.md): express roots as powers in readable text.
- [`simplify_fractions`](simplify_fractions.md): canonicalize TeX fractions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="simplify_fractions.html"><span class="label">Previous</span><span class="title">simplify_fractions</span></a>
  <a class="next-link" href="sqrt_pow.html"><span class="label">Next</span><span class="title">sqrt_pow</span></a>
</div>
