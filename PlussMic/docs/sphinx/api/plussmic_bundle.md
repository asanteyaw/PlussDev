# PlussMic `bundle`

The `bundle` model is the multi-table workspace surface.

- Use [`Locus`](../locus_api_reference.md) for a named workbench of related
  ragged `Catalog` values, declared links, pull plans, traversal, and SQL.
- Use [`LocusTraverse`](../locus_traverse_api_reference.md) for the join-graph
  builder returned by `Locus::from(...)`.
- Use [`LocusPull`](../locus_pull_api_reference.md) for the root-grain feature
  builder returned by `Locus::pull(root)`.
- Use [`LocusQuery`](../locus_query_api_reference.md) for SQL over catalogs
  returned by `Locus::query()`.
- Use [`Nexus`](../nexus_api_reference.md) for a named workbench of related
  `DataFrame` tables, declared relationships, join plans, and workspace audits.
- Use [`NexusTraverse`](../nexus_traverse_api_reference.md) for the fluent
  join-graph builder returned by `Nexus::from(...)`.
- Use [`NexusQuery`](../nexus_query_api_reference.md) for SQL-style workspace
  `SELECT` queries returned by `Nexus::query()`.
- Use [`NexusPull`](../nexus_pull_api_reference.md) for the fluent root-grain
  feature-table builder returned by `Nexus::pull(root)`.

```{toctree}
:maxdepth: 1

../locus_api_reference
../locus_traverse_api_reference
../locus_pull_api_reference
../locus_pull_agg_api_reference
../locus_query_api_reference
../locus_query_prepared_api_reference
../nexus_api_reference
../nexus_traverse_api_reference
../nexus_query_api_reference
../nexus_pull_api_reference
```
