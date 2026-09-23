# PlussMic `bundle`

The `bundle` model is the multi-table workspace surface.

- Use [`Nexus`](../nexus_api_reference.md) for a named workbench of related
  `DataFrame` tables, declared relationships, join plans, and workspace audits.
- Use [`NexusTraverse`](../nexus_traverse_api_reference.md) for the fluent
  join-graph builder returned by `Nexus::from(...)`.
- Use [`NexusQuery`](../nexus_query_api_reference.md) for SQL-style workspace
  `SELECT` queries returned by `Nexus::query()`.

```{toctree}
:maxdepth: 1

../nexus_api_reference
../nexus_traverse_api_reference
../nexus_query_api_reference
```
