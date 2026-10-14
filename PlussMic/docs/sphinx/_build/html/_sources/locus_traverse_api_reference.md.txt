<!-- pluss-hand-authored -->

# LocusTraverse API Reference

`LocusTraverse` is the immutable join-plan builder returned by
[`Locus::from(table)`](api/locus/from.md). It begins at one named catalog,
accumulates explicit or path-resolved joins, optionally projects the resulting
streams, and performs the work only when `collect()` is called.

The public workflow is:

1. Configure relational policy with `options()` when the Locus default is not appropriate.
2. Add one hop with `join()` or a shortest path with `via()`.
3. Set a final projection with `pick()` or `drop()`.
4. Materialize one ragged-native `Catalog` with `collect()`.

The builder owns a Locus handle rather than borrowing the caller's stack object,
so a chain created from a temporary Locus remains valid. Builder methods return a
new plan; they do not mutate the workspace or read catalog values before the
terminal call.

## Method Surface

| Method | Role |
|---|---|
| `collect()` | Execute the accumulated plan and return a `Catalog`. |
| `drop()` | Exclude named streams from the final result. |
| `join()` | Append one explicit or inferred relationship hop. |
| `options()` | Replace the relational matching and raggedness policy. |
| `pick()` | Keep named streams in the final result. |
| `via()` | Append the shortest declared path to another catalog. |

## Method Pages

```{toctree}
:hidden:
:maxdepth: 1

api/locus-traverse/collect
api/locus-traverse/drop
api/locus-traverse/join
api/locus-traverse/options
api/locus-traverse/pick
api/locus-traverse/via
```

- [`collect()`](api/locus-traverse/collect.md) executes the complete plan and
  returns its materialized Catalog.
- [`drop()`](api/locus-traverse/drop.md) removes named streams from the final
  schema.
- [`join()`](api/locus-traverse/join.md) appends one adjacent relationship or
  explicitly keyed join.
- [`options()`](api/locus-traverse/options.md) controls ragged alignment, loss
  handling, materialization, and key coalescing.
- [`pick()`](api/locus-traverse/pick.md) selects and orders the final streams.
- [`via()`](api/locus-traverse/via.md) appends the shortest declared path to a
  target catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="locus_api_reference.html"><span class="label">Previous</span><span class="title">Locus API Reference</span></a>
  <a class="next-link" href="api/locus-traverse/collect.html"><span class="label">Next</span><span class="title">collect</span></a>
</div>
