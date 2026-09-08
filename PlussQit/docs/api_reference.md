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
- [Timeline API Reference](sphinx/timeline_api_reference.md): the handwritten
  per-method reference for `Timeline` — the general temporal payload for scalar,
  vector, and N-D time data.
- [Timeline Factory API Reference](sphinx/timeline_factory_api_reference.md):
  construction, ranges, sampling, selection, and rendering in `pluss::timeline`.
- [Duration API Reference](sphinx/duration_api_reference.md): the handwritten
  per-method reference for `Duration` — the elapsed-time counterpart to
  `Timeline`, for `Timeline - Timeline` results and `Timeline ± Duration`
  arithmetic.
- [TimeIndex API Reference](sphinx/timeindex_api_reference.md): the handwritten
  per-method reference for `TimeIndex` — the 1D temporal axis object for search,
  slicing, resampling, rolling windows, and alignment.
- [Series Methods](Series_Methods.md): the naming-philosophy note (method-first vs.
  free functions, `pile`/`unite` naming deviations).
- [Method Index](coden_method_index.md): a flat cross-referenced index of the whole
  surface, documented and not-yet-documented alike.
- [Reductions](Reductions.md): sum/mean/min/max/std and related reduction behavior.
- [Statistics](Stats.md): statistical operations available on `Series`.

```{toctree}
:hidden:
:maxdepth: 1

sphinx/series_api_reference
sphinx/series_factory_api_reference
sphinx/note_api_reference
sphinx/note_factory_api_reference
sphinx/timeline_api_reference
sphinx/timeline_factory_api_reference
sphinx/duration_api_reference
sphinx/timeindex_api_reference
```

## Type system and semantics

- [API Philosophy](API_Philosophy.md): the design principles the whole surface follows.
- [Type Promotion](TypePromotion.md): how mixed-dtype operations resolve a result type.
- [Broadcasting](Broadcasting.md): shape-compatibility rules for elementwise operations.
- [NaN / Inf](NaN_Inf.md): how missing and non-finite values are represented and handled.
- [Complex](Complex.md), [Float8](Float8.md): specialized scalar type support.

## Indexing and shape

- [Indexing Parity](IndexingParity.md), [Indexing / Scatter](Indexing_Scatter.md): how
  element access and scatter-style writes behave.
- [Reshape and Views](ReshapeAndViews.md), [View Invariants](View_Invariants.md): what a
  reshape or view does and does not guarantee.
- [Scatter](Scatter.md), [Scatter/Reduce](ScatterReduce.md): scatter-with-reduction
  operations.

## Construction and configuration

- [Factories](Factories.md), [Factories / Config](Factories_Config.md): how `Series` and
  related objects are constructed.
- [Config](Config.md): library-wide configuration knobs.
- [Ownership](Ownership.md): value/reference ownership semantics.

## Errors

- [Error Handling](ErrorHandling.md): the exception and error-reporting model.
- [Errors / Stack Trace](Errors_StackTrace.md): diagnostic context attached to errors.

## Everything else

Every other `.md` file in this folder (design notes, patch logs, roadmap tracking) is
still reachable from the [documentation map](index.md) and searchable from this site's
search box, even though it isn't singled out above.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit Home</span>
  </a>
</div>
