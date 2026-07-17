Given the current split in [part01_orientation.md](/Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx/manual/part01_orientation.md:16) and the fact that `Atlas` is hard-strict in [AtlasInternals.inc](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/ops/atlas/AtlasInternals.inc:54), I would add a new table-layer object rather than relax `Atlas`.

**Positioning**
I would build a `FrameRegistry` as the first-class heterogeneous table workspace for PlussMic.

- `DataFrame` remains “one table.”
- `Atlas` remains “one table shape across depth.”
- `FrameRegistry` becomes “many different tables in one relational workspace.”
- `Catalog` stays Kindle-native: named streams, metadata, stream operations.
- `Registry` stays Bundle-native: named slots, heterogeneous record fields.

Long-term, the model should be:

```text
Thread -> DataFrame -> Atlas
            |
            -> FrameRegistry
```

That is deliberate. `FrameRegistry` is not a 3D tensor type and not a stream container. It is the table-layer database/workspace object.

**Intention**
Its job is not to replace `DataFrame`. Its job is to make multi-table work principled.

- Hold unrelated or loosely related `DataFrame`s without forcing schema compatibility.
- Carry relationship knowledge explicitly: keys, grain, cardinality, provenance.
- Let users do per-table work with normal `DataFrame` verbs.
- Add collection-level orchestration without forcing a SQL parser as the center of the world.

So the slogan would be:

- `DataFrame`: table algebra
- `Atlas`: panel algebra
- `FrameRegistry`: relational workspace algebra

**Implementation Plan**
I would do this in phases.

1. Phase 1: Core container
- Implement `FrameRegistry` as an ordered named collection of `DataFrame`.
- No shape restrictions, no row-count restrictions, no dtype restrictions across tables.
- Preserve deterministic table order.
- Support `defined`, `empty`, `size`, `keys`, `has`, `frame(name)`, `get(name)`, `set(name, df)`, `drop(name)`, `rename(name)`, `select(names)`.
- Add `map_frames`, `apply(name, fn)`, `apply_all`, `head_tables`, `tail_tables`, `summary`.
- Mirror PlussMic style: non-mutating methods by default, underscore mutators for in-place updates.
- Use the same handle-style implementation pattern as other PlussMic objects.

2. Phase 2: Metadata and schema graph
- Add `TableMeta` per table.
- `TableMeta` should include grain description, primary key, candidate keys, time field, case-label policy, source/provenance, and free-form notes.
- Add `Relationship` objects between tables.
- `Relationship` should include left table, right table, join columns, cardinality, optionality, confidence, and validation status.
- This is where the object becomes more than `unordered_map<string, DataFrame>`.

3. Phase 3: Validation and audit layer
- Add `validate_keys`, `validate_relationship`, `relationship_audit`, `detect_orphans`, `detect_duplicates`, `grain_audit`.
- Add inference helpers, but keep declarations explicit.
- Auto-inference should suggest relationships, not silently become truth.
- This phase is the real payoff for banking-style datasets.

4. Phase 4: Relational execution helpers
- Add explicit `JoinSpec`, `JoinPath`, and `JoinPlan`.
- Support `join(left, right, spec)` and `materialize(plan)`.
- Add `path(from, to)` over the relationship graph.
- Generate stable result names so users stop inventing ugly intermediate identifiers.
- Keep joins explicit; do not introduce magical implicit traversal.

5. Phase 5: Registry-aware query surface
- Add a method-driven query API over the registry first.
- Example direction: `registry.from("loan").join("account", ...).join("disp", ...).select(...).collect()`.
- Only after that is stable, add optional SQL over registry tables.
- SQL should be a front-end, not the semantic foundation.

6. Phase 6: Persistence and reproducibility
- Add manifest save/load for the registry.
- Persist tables plus schema graph plus provenance.
- Support fingerprints, schema snapshots, and relationship snapshots.
- This makes the object durable as a project workspace, not just a runtime bag.

7. Phase 7: Performance features
- Add lazy metadata recomputation.
- Add optional key indexes for join columns.
- Add caching for inferred relationship audits and join plans.
- Consider lazy-loading table handles later if registries get large.

**What I would not do early**
- I would not relax `Atlas`.
- I would not make `Catalog` pretend to be a table registry.
- I would not build a cost-based optimizer early.
- I would not add transactions, concurrency control, or storage-engine ambitions.
- I would not create a `FrameArchive` until there is a real multi-registry use case.

**How It Stacks Up Against SQL**
It will overlap with SQL in purpose, but it should not compete with SQL on SQL’s home turf.

Where SQL is stronger:
- declarative relational algebra
- mature optimizers
- indexes and storage engines
- transactions and concurrency
- standardization and ecosystem depth
- very large-scale query execution

Where `FrameRegistry` can be stronger:
- direct in-memory C++ composition
- no impedance mismatch between query result and object model
- seamless handoff to `DataFrame`, plotting, ML, and domain code
- explicit relationship metadata as a first-class object, not hidden in DDL
- programmable workflows where methods, lambdas, and typed APIs matter more than query strings

So I would position it like this:

- SQL is the database language and execution world.
- `FrameRegistry` is the in-process analytical workspace world.

That means the honest long-term ambition is not “beat SQL.”
It is:

- be better than ad hoc table maps,
- be cleaner than abusing `Atlas`,
- be more native than forcing everything through SQL strings,
- and interoperate with SQL when SQL is the right tool.

If you want, I can turn this into a concrete API draft next: constructors, method families, metadata structs, and the first 20 methods I’d actually ship.