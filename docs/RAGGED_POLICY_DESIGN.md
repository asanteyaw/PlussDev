# Ragged policy: one decision system for the kindle stack

Status: decisions confirmed (2026-10-08); steps 1-2 of the implementation order are done (see "What exists now"). Remaining work is listed under "Implementation order".

## Why this exists

A `Catalog` holds streams of different lengths. That is what separates the kindle stack from the table stack, and every operation has to decide what a missing record means. Today those decisions are made in several places with several vocabularies:

| Where | Vocabulary |
|---|---|
| `tabelize` | `TabelizePolicy`: `Align` (Strict/Inner/Outer/Left/Right), `AlignBy` (Pos/Label), `FillPolicy`, `DTypePolicy`, `CollisionPolicy` |
| cross-key reductions | `CatalogReduceOptions`: `align`, `align_by`, `skipnull`, `fill_scalar`, plus `ended`, `anchor`, `min_streams` |
| pairwise stats / plots | `PairAlign`, `PairAlignBy`, `XYAlignMode` |
| `Locus::join` / `audit` | `locus::Raggedness`, `JoinOptions` |

Consequences already visible in the code:

- `Catalog::merge` and the 3-D plot surfaces call `tabelize()` with its default (`Align::Inner`, by label), so ragged records are dropped silently.
- `Axel`, `Registry` and `Slot` (scribble) all delegate through `kindelize().tabelize(policy)`, so they inherit whatever kindle decides, but only for tabular paths.
- Each new operation (a reduction, a join, persistence) re-invents its own handling.

One policy system, consulted by every operation, fixes this. Users get one set of words, defaults they can predict, and explicit switches for everything else.

## Vocabulary: three different kinds of "missing"

The current code conflates these. The policy keeps them apart.

- **Absent**: the stream has no record at that index (it ended, or its labels skip it). A fact about shape.
- **Null**: the record exists but its value is invalid (a validity-mask bit, or NaN). A fact about data.
- **No partner**: a relational output row has no counterpart on one side. It materializes as null, but it is reported separately.

## The policy

`kindle::RaggedPolicy`, with orthogonal dimensions. Each operation reads only the ones that apply to it.

| Dimension | Values | Meaning |
|---|---|---|
| `domain` | Longest, Shortest, Exact | which indices exist in the result. Exact = error if lengths differ |
| `alignment` | Head, Tail, Label, Key(name) | what "the same index" means across streams |
| `absent` | Skip, Null, Fill(v), ForwardFill, Error | what an absent record does to the operation |
| `nulls` | Skip, Propagate | what an in-range null does |
| `min_support` | N (default 1) | fewest contributing streams for a value to exist |
| `materialize` | Preserve, Pad(fill) | for results that need not be rectangular: stay ragged, or pad |
| `loss` | Report, Error | Report always counts what was dropped/trimmed; Error refuses any loss |

Every operation that can lose or invent records returns a report (already done for joins: `JoinReport`).

## The decision tree: defaults per operation class

The defaults are chosen so the result is the least surprising for that kind of operation. Switching away from a default is always explicit.

| Operation class | domain | alignment | absent | nulls | materialize | Why |
|---|---|---|---|---|---|---|
| Cross-stream reduction (sum, mean, min, max, var, std...) | Longest | Head | Skip | Skip | Preserve | "reduce what is present"; shipped for reductions already |
| Elementwise between streams (a + b) | Longest | Head | Null | Skip | Preserve | an absent operand makes the value unknown |
| Join / merge / pull / traverse | per side: all records | Key | Null for no-partner | Skip (null keys never match) | Preserve | nothing dropped; keyless records reported; shipped for joins already |
| Pairwise statistics (corr, cov) | Shortest per pair | Head | Skip | Skip | n/a | statistics need complete pairs; matches `PairAlign::PairwiseInner` |
| Plotting | each series at its own extent | as given | Skip | Skip | n/a | draw what exists |
| To a rectangular consumer (tabelize, DataFrame, SQL, wide export) | Longest | Head | Null | keep | Pad(Null) | rectangular is required, so pad, and never trim |
| Persistence | exact | n/a | n/a | n/a | Preserve | store what is there; ragged-native format |

Two defaults change relative to today, and both are bug fixes:

1. `TabelizePolicy` default `Inner` becomes Longest + Pad(Null). `Inner` stays available as `domain = Shortest`.
2. `Catalog::merge` and the 3-D plot surfaces stop dropping silently (they follow from change 1, and `merge` should use the join engine).

Precedence: per-call argument, then the catalog's own policy (`catalog.ragged(policy)`), then the class default. The old option structs stay as sugar that converts to a `RaggedPolicy`, so existing code keeps compiling.

## Shared mechanics (this is where the performance comes from)

All operations need the same facts about a catalog, so compute them once.

- `RaggedLayout`: per-stream length, anchor offset, longest and shortest extent, and per-index presence counts (a difference array, O(streams + longest)). Cached on the catalog and invalidated by stream identity.
- Reduction kernel (already written for sum/mean/min/max/var/std): one pass per stream over the slice it covers, position blocks sized to stay in cache, blocks run in parallel. Next: fuse several reductions into one pass (sum + count + mean + var share the sweep).
- Join: key columns encoded to integers, one hash match, every stream rebuilt through the index pairs (already written). Next: cache the encodings and hash tables per (stream identity, mode) so repeated joins, audits and pulls on the same catalogs pay once.
- Zero-copy reads: use contiguous CPU `Float64` buffers directly instead of `to_vec` where possible.
- Measured so far: jagged reductions 290-450x faster than padding; native join 5.5-7x faster than the DataFrame join with tables already aligned, ~30x end to end at 4M records.

## Scribble

Scribble is the most general of the three stacks, so it needs its own discussion before it adopts anything beyond the tabular paths it already shares. Open questions to settle there:

- Are a slot's elements always flat, or can raggedness nest (ragged inside ragged)?
- Do heterogeneous element kinds in one axis change what "absent" and "null" mean?
- Which of the policy dimensions make sense per slot rather than per catalog?

Until then scribble keeps delegating through `kindelize().tabelize(policy)` and so picks up the new tabular defaults automatically.

## What exists now

Layering follows the library's rule (Stream first, Catalog delegates to Stream, Locus delegates to Catalog):

- `src/relational/` (namespace `pluss::relational`): the table-agnostic core shared by table, kindle and bundle: `Types` (links, plans, specs), `Links` (join-spec resolution, direct-link lookup that never guesses, cached path/plan search), `Match` (hash match on integer key codes, parallel probe, loss report).
- `kindle/Ragged.h`: `RaggedPolicy`, `OpClass` defaults, `RaggedLayout` (extents, offsets, window, presence counts), validation, and conversions to and from `CatalogReduceOptions` / `TabelizePolicy`.
- `Stream::gather`: rebuild a stream through a list of source records, keeping *absent*, *null* and *no partner* apart.
- `Catalog::join`, `join_detailed`, `audit_keys`, and `merge` (inner/left/right/outer, nothing dropped silently). Locus resolves links and delegates to these.
- Cross-stream reductions (sum/mean/min/max/var/std) run on `RaggedLayout` for every domain (Longest, Shortest, Exact) and both head and tail alignment, without padding.
- `TabelizePolicy` now pads by default (Longest, null fill); `Catalog::tabelize(RaggedPolicy)` enforces `Exact` and `loss = Error`. The 3-D surface plots ask for complete triples (`Align::Inner`) explicitly.

Decisions: the `tabelize` default changed (it was the silent-drop bug); elementwise ops give null for an absent operand while column-wise reductions keep "reduce what is present" with `absent = Null` as the switch; the refuse-any-loss switch is `loss = Error`.

Stale documentation to regenerate (verified examples): `api/catalog/tabelize.md` (still says `Inner` is the default), `api/catalog/merge.md` (still says only `inner` is supported), `api/catalog-plot3d/surface.md`.

## Implementation order

1. Done: `relational/` core, `kindle/Ragged.h`, tests pinning every row of the defaults table.
2. Done: reductions, `tabelize`, joins and `merge` on the policy; Locus delegates.
3. Next: native pull and traverse (move the pull grammar into `relational/` first), consulting the policy.
4. Ragged-native persistence (one file per stream plus labels; manifest records the extents).
5. Caching of encodings, hash tables and layouts; fused reductions; elementwise ops between streams on `OpClass::Elementwise`.
6. Locus API reference, and the stale Catalog pages above.
