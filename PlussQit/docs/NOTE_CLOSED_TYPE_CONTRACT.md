# Closed Note Type Contract

This document freezes the target contract for making `pluss::Note` a closed,
homogeneously typed value family. It is an implementation gate for the eight
migration passes, not public API teaching.

The priority order is strict:

1. semantic accuracy and validated failure behavior
2. vectorized and batched execution
3. allocation, locality, and throughput efficiency

An optimization that changes shape, dtype, broadcasting, ordering, encoding, or
failure behavior is not acceptable. A correct scalar loop is only an interim
reference: production kernels must then process whole Notes in batches and use
contiguous typed storage where the operation permits it.

## Why Note Is A Family

Text operations naturally produce more than text. A predicate produces boolean
lanes, a length produces integer lanes, and a similarity produces floating-point
lanes. Returning unrelated containers or a numeric `Series` breaks the Note
pipeline and loses the common shape and ownership model. Encoding booleans and
numbers as text preserves the class name but not the type contract.

The closed family therefore has five homogeneous dtypes:

| Dtype | Meaning | Required storage |
| --- | --- | --- |
| `Text` | UTF-8-oriented text cells | packed byte arena plus cell spans |
| `Bytes` | uninterpreted byte cells | packed byte arena plus cell spans |
| `Bool` | predicate and comparison lanes | contiguous `std::uint8_t` lanes |
| `Int64` | counts, positions, lengths, and ragged offsets | aligned contiguous `std::int64_t` lanes |
| `Float64` | similarities, proportions, and model scores | aligned contiguous `double` lanes |

`std::vector<bool>` is forbidden. Its proxy representation obstructs ordinary
contiguous access and predictable vectorized kernels.

## Return Boundary

Closed does not mean every member must return `Note`. Native C++ values remain
appropriate when the operation crosses an explicit object or host boundary.

| Operation class | Target return | Reason |
| --- | --- | --- |
| text/byte transformation | `Note` | preserves dtype and shape unless the operation explicitly reshapes |
| predicate or comparison | Bool `Note` | remains batchable and shape-aware |
| count, length, or position | Int64 `Note` | remains in the Note family without string conversion |
| similarity or score | Float64 `Note` | preserves numeric precision and batch execution |
| reshape, selection, or ordering | `Note` | preserves the receiver dtype |
| ragged tokenization | `TokenBatch` | structured result made from a Text `Note` and Int64 `Note` offsets |
| rank, extent, cardinality, dtype query | native scalar | describes the Note object itself, not one lane per cell |
| scalar/cell extraction | typed scalar or owning typed reference | explicit boundary from a Note to one host value |
| host export, iteration, streaming, debug rendering | native host type | explicit interoperability boundary |

`matrix()` and `slabs()` are decompositions, not host exports. Their target
forms are `std::vector<Note>` containing row or slab views that share storage
with the source. `host()` is the explicit owning conversion when nested host
containers are wanted.

## Semantic Invariants

- Every defined `Note` has exactly one dtype. Mixed-dtype storage is invalid.
- Shape metadata is independent of dtype and supports scalar through rank three,
  matching the existing public rank contract.
- Elementwise predicates, measurements, comparisons, and scores preserve the
  input shape. They do not flatten merely because the old return container was
  one-dimensional.
- Binary elementwise operations validate their documented shape or scalar-
  broadcast contract before allocating output.
- Text transforms accept `Text`; byte-safe transforms may also accept `Bytes`.
  Invalid dtype use fails immediately rather than parsing a rendered value.
- Bool and numeric kernels read native lanes. They never parse strings produced
  by another Note operation, and typed results are never formatted to text for
  internal transport.
- `Text` and `Bytes` remain distinct even when their bytes happen to be valid
  UTF-8. `is_bytes()` is the scalar dtype query. `is_utf8_valid()` is the
  shape-preserving content predicate.
- Undefined, scalar, empty, and zero-extent Notes remain distinct states. Each
  operation must specify and test its behavior for every applicable state.
- Views retain ownership of their backing allocation. A borrowed host view is
  valid only while its owning Note or owning cell reference remains alive.
- In-place operations preserve dtype and shape unless their public contract
  explicitly permits a shape mutation. They must not expose a partially updated
  receiver after validation failure.

## Accuracy Gates

Each migrated family requires a focused executable test before its old return
path is removed. Tests must cover all overloads, scalar and higher-rank shape,
empty and undefined inputs, dtype rejection, broadcasting, boundary values, and
stable errors. Existing text behavior is the compatibility oracle except where
the audit identifies an inconsistency such as flattened predicates or encoded
numeric output.

The machine-readable inventory is
`tools/audit/note_closed_type_contract.json`. It is generated from the current
declarations plus explicit records for handwritten and nonmember APIs. CMake's
`plussqit_coden_closed_note_contract_check` target rejects a stale inventory,
and the CodeN library depends on that check.

The inventory covers callable `Note` and `TokenBatch` operations and their
nonmember operators. Factories are a separate public family and will be migrated
with typed construction in Pass 3. Compiler-generated copy/move/destruction is
object lifecycle rather than a result operation and is intentionally excluded.

## Vectorization And Performance Gates

- A result kernel allocates its output storage once per Note, not once per cell.
- Bool, Int64, and Float64 results are written directly into contiguous lanes.
- Text kernels precompute or conservatively reserve output bytes and append into
  one packed arena; they do not construct one temporary Note per element.
- Predicate and numeric loops use batched contiguous traversal and are eligible
  for compiler vectorization. Branch-heavy Unicode work remains batched even
  when individual code-point decisions cannot be SIMD-vectorized.
- Ragged operations build one token arena and one contiguous Int64 offset Note.
- Row and slab decomposition is view construction; it does not copy cell bytes.
- Conversions to host containers occur only through explicit extraction APIs.
- Benchmarks record allocation count, bytes allocated, and throughput for small
  and large Notes. A pass cannot close with an unexplained material regression.

## Compatibility Reality

High-level PlussMic `DataFrame` and `Slate` operations can keep their public
contracts. Their internals can consume typed Notes or call explicit host export
at the boundary. Current production code relies mainly on `Note::vector()` for
text extraction, which remains available for Text and Bytes Notes.

Direct low-level callers are not source-compatible where a return type changes:
comparisons and predicates become Bool Notes, measurements and `argsort()` become
Int64 Notes, score methods become Float64 Notes, `matrix()` and `slabs()`
become Note decompositions, and `TokenBatch::offsets()` becomes an Int64 Note.
The two Series-returning compatibility helpers, `contains_series()` and
`lengths_bytes_series()`, are removed rather than retained as aliases.

NodeB and Ice must be rebuilt against the final public headers. The initial
downstream scan found no production use of those return-type escape hatches, but
the final pass still requires full builds and tests; absence in a text search is
not accepted as ABI proof.

## Eight Passes

| Pass | Scope | Acceptance condition |
| --- | --- | --- |
| 1 | contract and complete surface inventory | manifest is reproducible, CMake checks staleness, downstream risks are recorded |
| 2 | dtype tag and typed storage core | all five dtypes satisfy ownership, shape, copy/move, and alignment tests |
| 3 | factories, typed access, views, host export, and serialization | every dtype round-trips without text encoding; row/slab views share storage; versioned binary round trips preserve dtype, shape, and payload bits |
| 4 | Bool predicates and comparisons | exact shape/broadcast/error parity, then batched contiguous Bool kernels |
| 5 | Int64 measurements and Float64 scores | native precision and boundary behavior, then vectorized numeric output |
| 6 | `TokenBatch` and ragged offsets | tokens plus Int64 offsets validate segmentation without host containers |
| 7 | structural extraction | `matrix()` and `slabs()` decompose into storage-sharing Notes; `vector()` remains an explicit host boundary |
| 8 | downstream migration, benchmarks, documentation, and ABI cleanup | PlussQit, PlussMic, NodeB, and Ice pass; no legacy escape aliases remain |

Pass 1 deliberately changes no runtime behavior. It closes only when every
current public signature has a target disposition and the generated manifest is
identical to the checked-in contract.

## Implementation Status

| Pass | Status | Evidence |
| --- | --- | --- |
| 1 | complete | complete signature manifest is reproducible and enforced by the CodeN build; the exact count is generated rather than copied here |
| 2 | complete | five-state dtype tag, discriminated payload, checked shape/cardinality, 64-byte native lanes, zero-copy kernel-buffer adoption, normal and sanitizer tests |
| 3 | complete | typed scalar/vector/explicit-shape factories, checked native extraction, dtype-aware host/rendering, versioned `pack()`/`unpack()`, and storage-sharing row/slab views pass normal and sanitizer suites |
| 4 | complete | 17 content predicates, 18 comparison call forms, four logical operators, and two scalar Bool reductions use closed Bool Notes with exact-shape/scalar broadcasting |
| 5 | complete | all six measurement/count/ordering results are shape-preserving Int64 Notes, all 16 score results are Float64 Notes, and focused scalar, shaped, count, and ordering tests pass; generated count kernels write directly into one aligned Int64 result buffer, while `take(Int64 Note)` keeps `take(argsort())` inside the Note family and preserves the receiver dtype |
| 6 | complete | all 134 ragged signatures return `TokenBatch`; `tokens()` is a Text/Bytes Note and `offsets()` is an Int64 Note; token kernels move their aligned offset storage directly into the offset Note, and the central construction boundary rejects the wrong dtype, rank, origin, ordering, or terminal offset |
| 7 | complete | `matrix()` returns storage-sharing row Notes, `slabs()` returns storage-sharing slab Notes, and `vector()` remains the explicit borrowed-text host extraction boundary |
| 8 | complete for the closed-Note migration | the 529-signature audit and 54 focused normal tests pass; 1,054 assertions in 86 CodeN cases pass under ASan; all 37 Ice, NodeB, and unaffected PlussMic integration tests pass; the two Series escape aliases and their build option are absent from source and symbols; scoped allocation tests and the CPU benchmark cover small and large Bool/Int64 results |

The remaining `plussmic_table_csv` failure is not a closed-Note regression. Its
DataFrame median test expects the conventional midpoint `25` for
`{10, 20, 30, 40}`, while the current reduction delegates to the Series
`nanmedian()` order-statistic rule and returns `20`. That independent semantic
defect remains visible rather than weakening the test to close this migration.

## Pass 8 CPU Baseline

The current optimized CPU build, using 200,000 lanes and eight measured rounds,
records the following representative closed-result paths. Allocation counts and
bytes cover aligned native lane storage; the tracker is scoped and therefore
adds only one predictable null thread-local check when inactive.

| Kernel | Small throughput | Large throughput | Native allocations/call | Large bytes/call |
| --- | ---: | ---: | ---: | ---: |
| `contains()` -> Bool | 69.8 million lanes/s | 130.6 million lanes/s | 1 | 200,000 |
| Bool `operator|` | 130.7 million lanes/s | 2.85 billion lanes/s | 1 | 200,000 |
| `lengths_bytes()` -> Int64 | 175.6 million lanes/s | 2.61 billion lanes/s | 1 | 1,600,000 |

These figures are a local regression baseline, not a portable performance
promise. The executable at `examples/CodeN/coden_cpu_perf_bench.cpp` reproduces
the measurements and reports latency, throughput, allocation count, and bytes.
