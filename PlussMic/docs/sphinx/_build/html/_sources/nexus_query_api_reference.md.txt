# NexusQuery API Reference

This page documents the public `NexusQuery` surface — the fluent join-graph query
builder returned by [`Nexus::from(table)`](nexus_api_reference.md).

## What It Is

Calling `nexus.from("table_name")` returns a `NexusQuery` anchored at that table.
From there, `.join(other)` (optionally with a `JoinSpec`) or `.via(other, how)`
extends the walk across the join graph, `.pick()`/`.drop()` narrow the result
columns, `.name()` labels the result, and `.collect()` executes the accumulated plan
and returns the joined [`DataFrame`](dataframe_api_reference.md).

Every method except `.collect()` is non-mutating and chainable, returning a new
`NexusQuery` with one more step accumulated — nothing touches table data until
`.collect()` actually runs.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified
  outputs, and working previous/next navigation.

## Method Pages

The sidebar lists methods in typical pipeline order: `join`, `via`, `pick`, `drop`,
`name`, `collect`.

```{toctree}
:hidden:
:maxdepth: 1

api/nexus-query/join
api/nexus-query/via
api/nexus-query/pick
api/nexus-query/drop
api/nexus-query/name
api/nexus-query/collect
```
