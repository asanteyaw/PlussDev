<!-- pluss-hand-authored -->

# LocusPull API Reference

`LocusPull` is the root-grain feature builder returned by
[`Locus::pull(root)`](api/locus/pull.md). Plain `take()` specifications follow
root-preserving relationships, while reducers gather all related records back to
one value per root record. `collect()` is the terminal that executes the plan.

Calls such as `sum("orders.amount", "spent")` use a fully qualified source.
When several reducers come from one related catalog, `agg("orders")` enters the
[LocusPullAgg API Reference](locus_pull_agg_api_reference.md), where the table
prefix is bound once and `end()` returns to the owning pull.

The builder is intentionally mutable: methods append specifications and return a
reference to the same plan. That makes a named builder useful when a program adds
features conditionally. `root()` and `specs()` expose read-only planning metadata;
they do not execute a pull.

## Reference Data

The method pages use one permanent executable-backed workspace. Its
`customers` catalog has four records (`id`, `name`, `region_id`, `signup`),
`regions` maps region IDs 10, 20, and 30 to west, east, and north, and `events`
contains eight timestamped spends related many-to-one to customers. Customers
link many-to-one to regions. Primary keys and time fields are declared where
aggregate and window examples require them.

Output helpers print stream names and values directly from the resulting
`Catalog`, including validity masks as `null`. The complete source is
`PlussMic/examples/bundle/locus_pull_reference_demo.cpp`; every output block on
these pages comes from that executable.

## Method Surface

| Group | Methods |
|---|---|
| Plan construction | `add()`, `take()`, `agg()` |
| Reductions | `count()`, `ndistinct()`, `sum()`, `mean()`, `median()`, `min()`, `max()`, `std()`, `var()` |
| Policy | `options()` |
| Inspection | `root()`, `specs()` |
| Terminal | `collect()` |

## Method Pages

```{toctree}
:hidden:
:maxdepth: 1

api/locus-pull/add
api/locus-pull/agg
api/locus-pull/collect
api/locus-pull/count
api/locus-pull/max
api/locus-pull/mean
api/locus-pull/median
api/locus-pull/min
api/locus-pull/ndistinct
api/locus-pull/options
api/locus-pull/root
api/locus-pull/specs
api/locus-pull/std
api/locus-pull/sum
api/locus-pull/take
api/locus-pull/var
```

- [`add()`](api/locus-pull/add.md) appends explicit plain, aggregate, or
  windowed specifications.
- [`agg()`](api/locus-pull/agg.md) binds one related catalog for a sequence of
  scoped reducers.
- [`collect()`](api/locus-pull/collect.md) validates and executes the complete
  root-grain plan.
- [`count()`](api/locus-pull/count.md) counts usable related values.
- [`max()`](api/locus-pull/max.md) selects each group's upper numeric extreme.
- [`mean()`](api/locus-pull/mean.md) computes the arithmetic center.
- [`median()`](api/locus-pull/median.md) computes the order-statistic center.
- [`min()`](api/locus-pull/min.md) selects each group's lower numeric extreme.
- [`ndistinct()`](api/locus-pull/ndistinct.md) counts unique usable values.
- [`options()`](api/locus-pull/options.md) replaces raggedness and root-key
  policy.
- [`root()`](api/locus-pull/root.md) reports the catalog that owns the output
  grain.
- [`specs()`](api/locus-pull/specs.md) exposes the ordered stored request
  vector without executing it.
- [`std()`](api/locus-pull/std.md) computes sample standard deviation.
- [`sum()`](api/locus-pull/sum.md) totals usable numeric values.
- [`take()`](api/locus-pull/take.md) appends plain root or related-catalog
  streams through root-preserving paths.
- [`var()`](api/locus-pull/var.md) computes sample variance.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="locus_api_reference.html"><span class="label">Previous</span><span class="title">Locus API Reference</span></a>
  <a class="next-link" href="api/locus-pull/add.html"><span class="label">Next</span><span class="title">add</span></a>
</div>
