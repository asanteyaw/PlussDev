# NexusQuery API Reference

This page documents the public `NexusQuery` surface returned by
[`Nexus::query()`](nexus_api_reference.md).

`NexusQuery` lives in `pluss::bundle` beside `Nexus`, but it delegates to the
same in-memory SQLite-backed SQL engine used by
[`DataFrame::sql()`](dataframe_sql_accessor_api_reference.md). The difference is
scope: `DataFrame::sql()` starts from one frame, while `Nexus::query()` exposes
the named tables already loaded into the workspace.

## What It Is

Calling `nexus.query()` returns a SQL accessor. Use `.execute("SELECT ...")` for
one-off workspace queries, or `.prepare("SELECT ...")` when you want a reusable
statement template with runtime parameter binding.

This SQL surface is currently read-oriented and in-memory. It is for querying a
named table workspace, not for mutating persisted storage.

Because the underlying executor is now SQLite-backed, the workspace surface is no
longer limited to a hand-curated subset of `SELECT` syntax. Normal grouped queries,
CTEs, subqueries, `LEFT JOIN`, and similar query shapes are valid as long as the
statement produces a result set.

`.prepare("SELECT ...")` returns a `NexusQueryPrepared` — a separate small
type, also documented here, whose own `.execute(...)` overloads are what
actually run a prepared statement with bound parameters.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified
  outputs, and working previous/next navigation.

## Method Pages

The sidebar lists the two `NexusQuery` entry points (`execute`, `prepare`)
and `NexusQueryPrepared`'s own `execute`.

```{toctree}
:hidden:
:maxdepth: 1

api/nexus-sql/execute
api/nexus-sql/prepare
api/nexus-sql/prepared-execute
```
