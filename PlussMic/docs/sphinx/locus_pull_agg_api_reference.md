<!-- pluss-hand-authored -->

# LocusPullAgg API Reference

`LocusPullAgg` is the table-scoped reducer returned by
[`LocusPull::agg(table)`](api/locus-pull/agg.md). It borrows the owning
`LocusPull`, appends reducer specifications to that plan, and returns the owner
through `end()`. It is a temporary planning scope, not a standalone workspace
or result container.

Each reducer accepts one `(feature, alias)` pair or a batch of such pairs. The
scope supplies the catalog name; the feature remains relative to that catalog.
The supported reducers are `count`, `ndistinct`, `sum`, `mean`, `median`, `min`,
`max`, `std`, and `var`.

Every reducer has a scalar `(feature, alias)` overload and an
initializer-list overload of ordered `(feature, alias)` pairs. Feature names are
relative to the scoped catalog. Empty batches are no-ops; empty feature names
are rejected immediately. Catalog, relationship, stream-kind, duplicate-spec,
and output-alias validation occurs when the parent plan is collected.

The scope stores a borrowed parent pointer. Keep the `LocusPull` alive until all
scoped calls and `end()` have completed. Opening a scope and adding reducers
does not read a stream or allocate a result.

## Method Pages

```{toctree}
:hidden:
:maxdepth: 1

api/locus-pull-agg/count
api/locus-pull-agg/end
api/locus-pull-agg/max
api/locus-pull-agg/mean
api/locus-pull-agg/median
api/locus-pull-agg/min
api/locus-pull-agg/ndistinct
api/locus-pull-agg/std
api/locus-pull-agg/sum
api/locus-pull-agg/var
```

- [`count()`](api/locus-pull-agg/count.md) counts usable occurrences.
- [`end()`](api/locus-pull-agg/end.md) returns the exact parent builder.
- [`max()`](api/locus-pull-agg/max.md) adds upper-extreme reducers.
- [`mean()`](api/locus-pull-agg/mean.md) adds arithmetic means.
- [`median()`](api/locus-pull-agg/median.md) adds midpoint reducers.
- [`min()`](api/locus-pull-agg/min.md) adds lower-extreme reducers.
- [`ndistinct()`](api/locus-pull-agg/ndistinct.md) counts unique values.
- [`std()`](api/locus-pull-agg/std.md) adds sample standard deviations.
- [`sum()`](api/locus-pull-agg/sum.md) adds numeric totals.
- [`var()`](api/locus-pull-agg/var.md) adds sample variances.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="api/locus-pull/var.html"><span class="label">Previous</span><span class="title">LocusPull::var</span></a>
  <a class="next-link" href="api/locus-pull-agg/count.html"><span class="label">Next</span><span class="title">count</span></a>
</div>
