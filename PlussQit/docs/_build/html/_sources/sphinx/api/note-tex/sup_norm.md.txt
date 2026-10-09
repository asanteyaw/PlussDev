<!-- pluss-hand-authored -->
# Note::TexAccessor::sup_norm

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::sup_norm() const
```

## Brace Every Bare Superscript

`sup_norm()` wraps every unbraced superscript — `^x`, a run
of one or more ASCII letters/digits, or `^\alpha`, a single control word —
in explicit braces: `^{x}`, `^{\alpha}`. It is the exact `^` counterpart of
[`sub_norm`](sub_norm.md); every rule there —
the greedy alnum run, the untouched already-braced form, the whole-word
control-word capture — applies identically here with `^` in place of `_`.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: Bare, Control-Word, And Already-Braced Forms

```cpp
std::cout << pluss::make_note({"x^2 + y^ab + z^\\alpha + w^{already}"})
              .tex().sup_norm()
          << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["x^{2} + y^{ab} + z^{\\alpha} + w^{already}"]
```

## Example 2: The Greedy Run Captures Digit And Letter Together

```cpp
std::cout << pluss::make_note({"a^1b^2", "no sups here"})
              .tex().sup_norm()
          << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a^{1b}^{2}", "no sups here"]
```

Exactly the same greedy-run behavior documented on
[`sub_norm`](sub_norm.md) applies here: `1b` is
captured as one superscript, not split at the digit.

## Example 3: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().sup_norm().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().sup_norm();
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

- `sub_norm`: the identical bracing rule applied to `_`
  instead of `^`.
- `prime_norm`: rewrites trailing `'` marks into an explicit `^{...}`
  superscript.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="symbols.html"><span class="label">Previous</span><span class="title">symbols</span></a>
  <a class="next-link" href="tables.html"><span class="label">Next</span><span class="title">tables</span></a>
</div>
