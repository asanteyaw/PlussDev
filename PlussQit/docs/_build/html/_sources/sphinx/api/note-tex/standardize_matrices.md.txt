<!-- pluss-hand-authored -->
# NoteTexAccessor::standardize_matrices

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::standardize_matrices() const
```

## Rewrite Delimited Matrix Environments To `matrix`

`standardize_matrices()` rewrites every `\begin{...}`/`\end{...}` pair for
the six bracketed `amsmath` matrix environments — `bmatrix`, `pmatrix`,
`Bmatrix`, `vmatrix`, `Vmatrix`, and `smallmatrix` — to the plain,
delimiter-free `matrix` environment. Use it to compare or re-render matrix
source without caring which bracket style the author chose.

Only the environment markers change; the row/column content between them —
including `&` column separators and `\\` row separators — is copied
unchanged. This means the rewrite discards the visual delimiters
(`[`/`]` for `bmatrix`, `(`/`)` for `pmatrix`, `|`/`|` for `vmatrix`, and so
on): `standardize_matrices()` is for structural comparison, not for
producing a bracket-equivalent rendering. Any other environment name —
`array`, `tabular`, a custom matrix environment — is not recognized and
passes through untouched.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: Two Different Bracket Styles

```cpp
std::cout << pluss::make_note(
    {"\\begin{pmatrix}1&0\\\\0&1\\end{pmatrix} and "
     "\\begin{vmatrix}a&b\\\\c&d\\end{vmatrix}"})
    .tex().standardize_matrices()
    << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\\begin{matrix}1&0\\\\0&1\\end{matrix} and \\begin{matrix}a&b\\\\c&d\\end{matrix}"]
```

The `&` column separators and `\\` row separator inside each environment are
untouched; only the `\begin{...}`/`\end{...}` markers change.

## Example 2: An Unrecognized Environment Is Untouched

```cpp
std::cout << pluss::make_note({"\\begin{Bmatrix}x\\end{Bmatrix}", "no matrix here"})
              .tex().standardize_matrices()
          << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["\\begin{matrix}x\\end{matrix}", "no matrix here"]
```

## Example 3: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().standardize_matrices().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().standardize_matrices();
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

- `standardize_vectors`: the equivalent canonicalization for vector
  notation.
- `envs` / `env_names`: inventory environments generically rather than
  rewriting a fixed set of matrix names.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="standardize_vectors.html"><span class="label">Previous</span><span class="title">standardize_vectors</span></a>
  <a class="next-link" href="standardize_integrals.html"><span class="label">Next</span><span class="title">standardize_integrals</span></a>
</div>
