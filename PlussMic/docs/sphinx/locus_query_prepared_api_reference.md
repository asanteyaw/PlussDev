<!-- pluss-hand-authored -->

# LocusQueryPrepared API Reference

`LocusQueryPrepared` owns a Locus workspace, a statement template, and the SQL
options captured when [`Locus::query(options)`](api/locus/query.md) was called.
Its [`execute()`](api/locus-query-prepared/execute.md) overloads either run the
template unchanged or merge runtime value and identifier bindings over those
captured defaults.

Preparation stores a reusable recipe; schema lookup and SQL compilation happen
at execution. This means a prepared object is cheap to construct, but an unknown
catalog, unknown stream, or malformed statement is reported by `execute()`, not
by `prepare()`.

```{toctree}
:hidden:
:maxdepth: 1

api/locus-query-prepared/execute
```

