<!-- pluss-hand-authored -->
# Note Regex Accessor API Reference

[Back to Note API Reference](note_api_reference)

`Note::regex()` is the pattern-processing boundary for Note. The family has
six canonical operations, each answering a different question about the same
ECMAScript regular-expression language.

| Need | Method | Result |
|---|---|---|
| Count non-overlapping matches per cell | `count` | Shape-preserving Int64 Note |
| Extract every non-overlapping match | `findall` | `TokenBatch` |
| Validate the complete contents of each cell | `match` | Shape-preserving Bool Note |
| Rewrite every non-overlapping match | `replace` | Shape-preserving Text Note |
| Test whether any substring matches | `search` | Shape-preserving Bool Note |
| Cut each cell at matching delimiters | `split` | `TokenBatch` |

`findall()` and `split()` are not aliases. The first returns the spans that
match; the second returns the text between matching spans. Likewise,
`match()` requires the pattern to cover a complete cell, while `search()` only
requires one occurrence. These distinctions are why the family has separate
methods rather than one mode-heavy call.

The accessor deliberately has no `tokens()` synonym for `findall()`. Language
tokenization belongs to `Note::nlp()`, literal field splitting belongs to
`Note::split()`, and pattern-defined extraction belongs here.

```{toctree}
:hidden:
:maxdepth: 1

api/note-regex/count
api/note-regex/findall
api/note-regex/match
api/note-regex/replace
api/note-regex/search
api/note-regex/split
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="api/note-regex/split.html"><span class="label">Previous</span><span class="title">split</span></a>
  <a class="next-link" href="api/note-regex/count.html"><span class="label">Next</span><span class="title">count</span></a>
</div>
