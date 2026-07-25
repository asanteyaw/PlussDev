<!-- pluss-hand-authored -->

# PlussQit API Reference

[Back to PlussQit](index.md)

PlussQit's method surface is documented across a set of focused design and reference
notes. This page is the curated entry point into them.

## Method surface

- [Series Methods](Series_Methods.md): the method inventory for `Series`.
- [Method Index](coden_method_index.md): a cross-referenced index of methods by name.
- [Reductions](Reductions.md): sum/mean/min/max/std and related reduction behavior.
- [Statistics](Stats.md): statistical operations available on `Series`.

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
