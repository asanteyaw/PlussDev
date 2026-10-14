<!-- pluss-hand-authored -->

# Locus API Reference

This page documents the public `Locus` surface as part of the `bundle` workspace
model.

`Locus` is a named, heterogeneous workspace of [`Catalog`](catalog_api_reference.md)s
with declared relationships -- the `Catalog` counterpart of
[`Nexus`](nexus_api_reference.md), which coordinates `DataFrame`s. Native Locus joins,
pulls, traversal, plans, and integrity checks preserve Catalog and Stream semantics;
their matching and gathering work runs through the shared relational and Catalog
kernels rather than forcing ragged data through a rectangular frame. `nexus()` remains
the explicit aligned bridge for DataFrame-oriented workflows, reports, and SQL.

Use the `pluss::bundle` namespace for `Locus` itself, and `pluss::kindle` for the
`Catalog` objects it coordinates.

```{note} No dedicated constructor page
Construct a `Locus` with `Locus({{"name", catalog}, ...})` (a vector or brace list of
name/`Catalog` pairs); there is no dedicated constructor page, the same as `Nexus`'s
own constructors. The two named factories, `from_archive` and `from_nexus`, each have
their own page.
```

```{note} Full surface coverage
`Locus` has roughly ninety public members across six classes (`Locus`, `LocusPull`,
`LocusPullAgg`, `LocusTraverse`, `LocusQuery`, `LocusQueryPrepared`). Every one has a
method page: this page's own tranche below covers `Locus` itself; `LocusPull` and
`LocusPullAgg` have their own landing pages
([`LocusPull`](locus_pull_api_reference.md), [`LocusPullAgg`](locus_pull_agg_api_reference.md));
and `LocusTraverse`, `LocusQuery`, and `LocusQueryPrepared` have theirs
([`LocusTraverse`](locus_traverse_api_reference.md), [`LocusQuery`](locus_query_api_reference.md),
[`LocusQueryPrepared`](locus_query_prepared_api_reference.md)).
```

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signature, parameter details, notes, verified
  outputs, and working previous/next navigation within this tranche.
- This landing page stays intentionally short so the content panel is not one giant
  scroll of every method.

## Method Pages

- [`construction`](api/locus/construction.md) -- `from_archive`, `from_nexus`
- [`state`](api/locus/state.md) -- `defined`, `unique`, `empty`, `ntables`,
  `nrelationships`, `shape`
- [`catalogs`](api/locus/catalogs.md) -- `keys`, `catalogs`, `has`, `catalog`, `get`
- [`head`](api/locus/head.md) -- `head`, `tail`
- [`select`](api/locus/select.md) -- `select`
- [`drop`](api/locus/drop.md) -- `drop`
- [`with`](api/locus/with.md) -- `with`, `with_`
- [`inject`](api/locus/inject.md) -- `inject`, `inject_`
- [`without`](api/locus/without.md) -- `without`, `without_`
- [`rename`](api/locus/rename.md) -- `rename`, `rename_`
- [`apply`](api/locus/apply.md) -- `apply`, `apply_`
- [`apply_all`](api/locus/apply_all.md) -- `apply_all`
- [`flow`](api/locus/flow.md) -- `flow`
- [`has_table_meta`](api/locus/has_table_meta.md) -- `has_table_meta`, `table_meta`
- [`meta`](api/locus/meta.md) -- `meta` (getter and setter), `meta_`
- [`set_table_meta`](api/locus/set_table_meta.md) -- `set_table_meta`, `set_table_meta_`
- [`link`](api/locus/link.md) -- `link` (both overloads), `add_relationship`,
  `add_relationships`, `unlink`, `has_relationship`, `relationship`, `relationships`,
  `links`, `neighbors`, `path`
- [`audit`](api/locus/audit.md) -- `audit` (both overloads), `validate`,
  `validate_keys`, `duplicates`, `validate_relationship`, `check`, `detect_orphans`
- [`policy`](api/locus/policy.md) -- `default_policy`, `policy`, `with_policy`,
  `with_policy_`
- [`nexus`](api/locus/nexus.md) -- `nexus`
- [`alignment_cache_stats`](api/locus/alignment_cache_stats.md) -- `alignment_cache_stats`
- [`join`](api/locus/join.md) -- `join` (both overloads), `join_detailed`
- [`audit_report`](api/locus/audit_report.md) -- `audit_report`
- [`grain_audit`](api/locus/grain_audit.md) -- `grain_audit`
- [`relationship_audit`](api/locus/relationship_audit.md) -- `relationship_audit`
- [`summary`](api/locus/summary.md) -- `summary`
- [`profile`](api/locus/profile.md) -- `profile`
- [`scan`](api/locus/scan.md) -- `scan`
- [`deduce`](api/locus/deduce.md) -- `deduce`
- [`schema`](api/locus/schema.md) -- `schema`
- [`graph`](api/locus/graph.md) -- `graph`
- [`fingerprint`](api/locus/fingerprint.md) -- `fingerprint`
- [`plan`](api/locus/plan.md) -- `plan` (both overloads)
- [`materialize`](api/locus/materialize.md) -- `materialize`
- [`has_store`](api/locus/has_store.md) -- `has_store`, `stores`
- [`store`](api/locus/store.md) -- `store` (getter and setter), `store_`
- [`run`](api/locus/run.md) -- `run` (both overloads)
- [`persistence`](api/locus/persistence.md) -- `save`, `load`, `read`
- [`stream`](api/locus/stream.md) -- `to_string`, `show`, `operator<<`
- [`pull`](api/locus/pull.md) -- `Locus::pull()`, the entry point into the
  `LocusPull` root-grain feature builder
- [`from`](api/locus/from.md) -- `Locus::from()`, the entry point into the
  `LocusTraverse` join-walking builder
- [`query`](api/locus/query.md) -- `Locus::query()`, the entry point into the
  `LocusQuery` SQL workspace

```{toctree}
:hidden:
:maxdepth: 1

api/locus/construction
api/locus/state
api/locus/catalogs
api/locus/head
api/locus/select
api/locus/drop
api/locus/with
api/locus/inject
api/locus/without
api/locus/rename
api/locus/apply
api/locus/apply_all
api/locus/flow
api/locus/has_table_meta
api/locus/meta
api/locus/set_table_meta
api/locus/link
api/locus/audit
api/locus/policy
api/locus/nexus
api/locus/alignment_cache_stats
api/locus/join
api/locus/audit_report
api/locus/grain_audit
api/locus/relationship_audit
api/locus/summary
api/locus/profile
api/locus/scan
api/locus/deduce
api/locus/schema
api/locus/graph
api/locus/fingerprint
api/locus/plan
api/locus/materialize
api/locus/has_store
api/locus/store
api/locus/run
api/locus/persistence
api/locus/stream
api/locus/pull
api/locus/from
api/locus/query
```

<div class="pluss-inline-prev-next">
  <a class="next-link" href="api/locus/construction.html">
    <span class="label">Next</span>
    <span class="title">construction</span>
  </a>
</div>
