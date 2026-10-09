<!-- pluss-hand-authored -->
# Note::TexAccessor::canonicalize_macros

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::canonicalize_macros() const
```

## Normalize Equivalent Built-In Command Spellings

`canonicalize_macros()` rewrites a small, explicit set of legacy or stylistic
TeX control words to one canonical spelling. Use it before comparison,
indexing, or another source transform when equivalent notation should no
longer differ merely because one cell used `\dfrac` and another used `\frac`.

The complete mapping is:

| Input control word | Canonical output |
| --- | --- |
| `\dfrac`, `\tfrac`, `\cfrac` | `\frac` |
| `\operatorname` | `\mathrm` |
| `\rm` | `\mathrm` |
| `\bf` | `\mathbf` |
| `\it` | `\mathit` |
| `\cal` | `\mathcal` |
| `\Bbb` | `\mathbb` |
| `\lt`, `\gt` | `<`, `>` |
| `\leqslant` | `\leq` |

Matching is by complete control-word name, not byte prefix. Thus canonical
`\mathrm` remains `\mathrm`, `\item` is not mistaken for legacy `\it`, and an
unrelated command such as `\ltd` is not shortened to `<d`. Replacements are
performed in one lexical pass and are not fed back through the mapping, which
makes the result idempotent.

This method does not inspect macro declarations, expand user-defined macros,
rewrite arguments, or decide whether two arbitrary commands render alike.
Use `macro_subs()` for declarations and use sites; use
[`collapse_macros()`](collapse_macros.md) when declarations should be removed
without expanding their uses.

The result is a newly owned Text Note with the same rank, shape, and cell
order as the receiver. Undefined propagates. Empty cells and cells without a
mapped control word are copied unchanged. Bytes and native Bool, Int64, or
Float64 Notes reject. The lexical pass is linear in total source bytes and
does not compile a regular expression per cell.

## Example 1: Every Canonical Family

```cpp
const auto source = pluss::make_note({
    R"(\dfrac{1}{2} + \tfrac{a}{b} + \cfrac{x}{y})",
    R"(\operatorname{rank} \rm old \bf b \it i \cal C \Bbb R)",
    R"(\mathrm{kept} \mathbf{v} \item \ltd \leqslant \lt \gt)"});
const auto canonical = source.tex().canonicalize_macros();
std::cout << canonical << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\frac{1}{2} + \\frac{a}{b} + \\frac{x}{y}", "\\mathrm{rank} \\mathrm old \\mathbf b \\mathit i \\mathcal C \\mathbb R", "\\mathrm{kept} \\mathbf{v} \\item \\ltd \\leq < >"]
```

The third cell demonstrates the command boundary: `\mathrm`, `\item`, and
`\ltd` survive exactly, while the complete `\leqslant`, `\lt`, and `\gt`
commands change.

## Example 2: Canonicalization Is Idempotent

```cpp
std::cout << std::boolalpha << "idempotent="
          << (canonical.tex().canonicalize_macros() == canonical)
                 .all().bool_item()
          << '\n';
```

### Output

```text
idempotent=true
```

## Example 3: Native Integer Cells Reject

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().canonicalize_macros();
} catch (const pluss::Error&) {
  std::cout << "canonical Int64: rejected\n";
}
```

### Output

```text
canonical Int64: rejected
```

## See Also

- [`collapse_macros`](collapse_macros.md): remove declarations and canonicalize the remaining source.
- [`macro_subs`](macro_subs.md): substitute locally declared macro bodies at use sites.
- `cmd_sub`: rewrite one caller-selected command.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bibliography_keys.html"><span class="label">Previous</span><span class="title">bibliography_keys</span></a>
  <a class="next-link" href="captions.html"><span class="label">Next</span><span class="title">captions</span></a>
</div>
