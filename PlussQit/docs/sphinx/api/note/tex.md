<!-- pluss-hand-authored -->
# Note::tex

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
NoteTexAccessor Note::tex() const noexcept
```

## Enter The TeX/LaTeX Operation Surface

`tex()` binds a lightweight accessor to the current Note. The accessor
groups TeX/LaTeX-source cleanup, math-delimiter handling, construct removal,
environment extraction, macro expansion, math simplification and
standardization, format conversion, structural and semantic extraction, and
command/symbol replacement without crowding the general Note surface with
`tex_`-prefixed method names.

The call does not parse TeX, allocate storage, copy the Note, or change its
dtype. It returns a temporary proxy containing one pointer to the receiver;
the selected operation performs the work. This keeps `Note` itself a
one-pointer intrusive handle while allowing fluent pipelines:

```cpp
const auto cleaned = source.tex().strip_comments().tex().strip_labels();
const auto plain = cleaned.tex().to_text();
```

The accessor is valid for the full expression in which it is created. Store
the Note, not the accessor, when work must outlive that expression. Each
accessor method documents its own accepted dtype, shape behavior, options,
and result type.

[Open the Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Example: Strip Then Convert

```cpp
const auto source = pluss::make_note(
    "\\documentclass{article}\n"
    "% draft note\n"
    "\\begin{document}\n"
    "Body \\textbf{bold} text.\n"
    "\\end{document}\n");
const auto stripped = source.tex().strip_comments().tex().strip_preamble();
const auto plain = stripped.tex().to_text();

std::cout << pluss::note::print(plain) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"Body bold text."
```

The comment and the class/`\begin{document}`/`\end{document}` scaffolding
are gone after the first two calls; `to_text()` then unwraps `\textbf{bold}`
into plain `bold`.

## See Also

- [Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)
- [`nlp`](nlp.md): linguistic analysis and corpus cleanup, outside TeX-source
  handling.
- [`regex`](regex.md): explicit pattern operations outside TeX-aware parsing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html"><span class="label">Previous</span><span class="title">take</span></a>
  <a class="next-link" href="title.html"><span class="label">Next</span><span class="title">title</span></a>
</div>
