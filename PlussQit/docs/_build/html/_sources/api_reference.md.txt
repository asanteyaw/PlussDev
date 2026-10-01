<!-- pluss-hand-authored -->

# PlussQit API Reference

[Back to PlussQit](index.md)

PlussQit's method surface is documented across a set of focused design and reference
notes. This page is the curated entry point into them.

## Method surface

- [Series API Reference](sphinx/series_api_reference.md): the per-method reference
  for `Series` — one page per method, with real, verified examples. The primary
  entry point; start here.
- [Series Factory API Reference](sphinx/series_factory_api_reference.md): the
  separate handwritten construction reference for `pluss::series::*` — shape-first
  allocators, value-first constructors, range builders, and the broader Series
  factory surface.
- [Note API Reference](sphinx/note_api_reference.md): the handwritten per-method
  reference for `Note` — the vectorized text object for cleanup, tokenization,
  regex work, NLP-style text preprocessing, and TeX-aware transforms.
- [Note Factory API Reference](sphinx/note_factory_api_reference.md): the
  separate handwritten construction reference for `pluss::note::*` — text and
  bytes constructors, shape-first and like-factories, generative builders, and
  text-splitting and assembly factories.
- [TokenBatch API Reference](sphinx/tokenbatch_api_reference.md): the closed
  ragged text result returned by tokenizers and extractors, including flat
  token storage, row offsets, state, and ownership.
- [Timeline API Reference](sphinx/timeline_api_reference.md): the handwritten
  per-method reference for `Timeline` — the general temporal payload for scalar,
  vector, and N-D time data.
- [Timeline Factory API Reference](sphinx/timeline_factory_api_reference.md):
  construction, ranges, sampling, selection, and rendering in `pluss::timeline`.
- [Duration API Reference](sphinx/duration_api_reference.md): the handwritten
  per-method reference for `Duration` — the elapsed-time counterpart to
  `Timeline`, for `Timeline - Timeline` results and `Timeline ± Duration`
  arithmetic.
- [Duration Factory API Reference](sphinx/duration_factory_api_reference.md):
  exact unit construction, progressions, shape-first allocation, sampling, and
  rendering in `pluss::duration`.
- [Timeline Index Accessor API Reference](sphinx/timeline_index_accessor_api_reference.md):
  the ordered-axis methods entered through `Timeline::index()` for search,
  slicing, resampling, rolling windows, and alignment.
- [TimeIndex Factory API Reference](sphinx/timeindex_factory_api_reference.md):
  ordered-axis construction, regular ranges, buffer ownership, and rendering
  in `pluss::timeindex`.
- [Method Index](coden_method_index.md): a flat cross-referenced index of the whole
  surface, documented and not-yet-documented alike.

```{toctree}
:hidden:
:maxdepth: 1

sphinx/series_api_reference
sphinx/series_factory_api_reference
sphinx/note_api_reference
sphinx/note_factory_api_reference
sphinx/tokenbatch_api_reference
sphinx/timeline_api_reference
sphinx/timeline_factory_api_reference
sphinx/duration_api_reference
sphinx/duration_factory_api_reference
sphinx/timeline_index_accessor_api_reference
sphinx/timeindex_factory_api_reference
```

The family references are the public documentation. Historical omnibus pages,
code-generation notes, patch logs, and authoring standards remain in the source tree
for maintainers but are intentionally excluded from the published site.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit Home</span>
  </a>
</div>
