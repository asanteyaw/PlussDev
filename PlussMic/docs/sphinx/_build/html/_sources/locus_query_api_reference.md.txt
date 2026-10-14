<!-- pluss-hand-authored -->

# LocusQuery API Reference

`LocusQuery` is the SQL accessor returned by
[`Locus::query()`](api/locus/query.md). Every catalog key becomes a table name in
one in-memory workspace. Query results cross back into the ragged layer as a
defined `Catalog`; a result with no matching records still retains its projected
stream names.

Use [`execute()`](api/locus-query/execute.md) for one statement run. Use
[`prepare()`](api/locus-query/prepare.md) when statement text is reused with
different value or identifier bindings; it returns the separately documented
[LocusQueryPrepared API Reference](locus_query_prepared_api_reference.md).

The accessor owns a Locus-derived workspace handle. It does not borrow the local
variable on which `query()` was called, so storing an accessor or prepared query
created from a temporary Locus is safe.

```{toctree}
:hidden:
:maxdepth: 1

api/locus-query/execute
api/locus-query/prepare
```

