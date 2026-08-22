# NexusTraverse API Reference

This page documents the public `NexusTraverse` surface — the fluent join-graph
builder returned by [`Nexus::from(table)`](nexus_api_reference.md).

`NexusTraverse` lives in `pluss::bundle` beside `Nexus`, even though the joined
result it eventually materializes is still a table-side
[`DataFrame`](dataframe_api_reference.md).

## What It Is

Calling `nexus.from("table_name")` returns a `NexusTraverse` anchored at that
table. From there, `.join(other)` (optionally with a `JoinSpec`) or
`.via(other, how)` extends the walk across the join graph, `.pick()`/`.drop()`
narrow the result columns, `.name()` labels the result, and `.collect()`
executes the accumulated plan and returns the joined
[`DataFrame`](dataframe_api_reference.md).

Every method except `.collect()` is non-mutating and chainable, returning a new
`NexusTraverse` with one more step accumulated.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, and
  verified outputs.

## Method Pages

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
