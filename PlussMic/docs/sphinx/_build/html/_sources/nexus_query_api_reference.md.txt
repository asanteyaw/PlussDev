# NexusQuery API Reference

This page documents the public `NexusQuery` surface — the fluent join-graph query
builder returned by [`Nexus::from(table)`](nexus_api_reference.md).

```{important} Coming soon
`NexusQuery` method pages are not written yet — this landing page is a placeholder
reserved as part of building out the [`Nexus`](nexus_api_reference.md) reference.
```

## What It Is

Calling `nexus.from("table_name")` returns a `NexusQuery` anchored at that table.
From there, `.join(other)` (optionally with a `JoinSpec`) or `.via(other, how)`
extends the walk across the join graph, `.pick()`/`.drop()` narrow the result
columns, `.name()` labels the result, and `.collect()` executes the accumulated plan
and returns the joined [`DataFrame`](dataframe_api_reference.md).
