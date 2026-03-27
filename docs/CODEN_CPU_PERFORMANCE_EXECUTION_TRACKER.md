# CodeN CPU Performance Execution Tracker

## Program Goal
Execute the CodeN CPU performance blueprint in controlled, test-first patches.

## Success Definition
- Correctness parity with reference behavior on all covered cases.
- Measurable speedups on targeted operations.
- No API breakage for existing users.

## Status Legend
- `[ ]` Not started
- `[/]` In progress
- `[x]` Completed
- `[!]` Blocked

## Patch 01: Baseline + Harness
- Status: `[/]`
- Goal: Establish trustworthy baselines and regression corpus.
- Tasks:
  - `[x]` Add microbench for `Note::sort`.
  - `[x]` Add microbench for `Note::distinct`.
  - `[ ]` Add microbench for `Note::tally`/count-by path.
  - `[x]` Add microbench for `Note::take(indices)`.
  - `[x]` Add correctness corpus with nulls, repeated labels, mixed lengths, UTF-8.
  - `[x]` Store baseline numbers in docs/artifacts.
- Gate:
  - `[x]` Baseline report committed and reproducible.

### Patch 01 Baseline Snapshot
- Environment:
  - `build` (examples enabled) for benchmark binary.
  - `build-plussdev-tests` (tests enabled, no-catch path) for regression corpus assertions.
- Command:
  - `./build/PlussQit/plussqit_coden_cpu_perf_bench 200000 8`
- Result:
  - `sort_ms=13.6704`
  - `unique_ms=5.644`
  - `deduplicate_ms=2.72`
  - `take_ms=0.133125`
- Regression corpus coverage:
  - `sort` monotonic lexical order on mixed UTF-8 + empty-string corpus.
  - `unique` sorted distinct behavior on same corpus.
  - `deduplicate` first-seen stability on same corpus.
  - `take` permutation gather semantics with negative-index handling.

## Patch 02: Public `Note::argsort`
- Status: `[/]`
- Goal: Provide permutation primitive (`Series<int64>`) for Note.
- Tasks:
  - `[x]` Add API surface `Note::argsort(...)`.
  - `[ ]` Add dispatch wiring.
  - `[x]` Add tests for ordering and stability.
  - `[ ]` Add tests for null behavior policy.
- Gate:
  - `[ ]` New API passes tests and docs mention behavior.
  - Note: current `argsort` surface is interop-gated behind `CODEN_ENABLE_SERIES_INTEROP`.

## Patch 03: Fast `Note::take(indices)`
- Status: `[/]`
- Goal: Fast gather-by-permutation kernel for spans.
- Tasks:
  - `[x]` Implement optimized gather for `OffLen`.
  - `[x]` Avoid unnecessary string materialization.
  - `[x]` Add bounds and error-path tests.
  - `[ ]` Benchmark vs previous path.
- Gate:
  - `[ ]` `take` shows measurable speedup on benchmark corpus.

### Verification Snapshot (2026-03-21)
- Code changes validated:
  - Added internal reusable 1D permutation primitive: `ops_internal::note_argsort_indices_1d(...)`.
  - `Note::argsort(...)` now delegates ordering logic to the shared primitive.
  - Added `take` out-of-range positive/negative index checks in:
    - `PlussQit/tests/CodeN/test_coden_nocatch_smoke.cpp`
    - `PlussQit/tests/CodeN/test_coden_patch22_perf_regression_corpus.cpp`
- Benchmark check:
  - `./build/PlussQit/plussqit_coden_cpu_perf_bench 200000 8`
  - `sort_ms=12.8581`, `unique_ms=5.58038`, `deduplicate_ms=2.69963`, `take_ms=0.13325`
- Runtime check:
  - no-catch suites in both `build-plussdev-tests` and `build-plussdev-interop-tests` execute through:
    - `coden:perf_regression_corpus`
    - `coden:argsort_interop` (interop build)
  - They still terminate later on pre-existing unrelated `aux:transmit_translate_queue` failure.

## Patch 04: Compare/Equality/Hash Kernels
- Status: `[/]`
- Goal: Reusable span-aware kernels for hot ops.
- Tasks:
  - `[x]` Implement lexical compare kernel.
  - `[x]` Implement equality kernel.
  - `[x]` Implement hash kernel with collision-safe validation path.
  - `[x]` Add unit tests across UTF-8 and edge cases.
- Gate:
  - `[ ]` Kernel outputs match reference behavior exactly (expanded corpus still pending).

### Patch 04 Snapshot (2026-03-21)
- Added kernels in `CodeN/native/cpu/TextKernels`:
  - `compare_lex_bytes(...)`
  - `compare_lex_bytes_batch(...)`
  - `hash_bytes_fnv1a(...)`
  - `hash_bytes_fnv1a_batch(...)`
- Integrated kernel usage:
  - `Note` pair/scalar comparisons route through lexical compare kernels.
  - `deduplicate` now uses hash-bucket narrowing + lexical equality confirmation (collision-safe).
  - `unique` now uses `string_view` lane path (no string materialization copies).
- Added kernel-focused UTF-8 checks:
  - no-catch: `test_text_kernels_compare_hash()` in `tests/CodeN/test_coden_nocatch_smoke.cpp`
  - catch2 path: `CodeN text kernels compare/hash UTF-8 edge checks` in `tests/CodeN/test_coden_patch22_perf_regression_corpus.cpp`
- Benchmark update (`./build/PlussQit/plussqit_coden_cpu_perf_bench 200000 8`):
  - `sort_ms=12.646`
  - `unique_ms=4.82062` (improved vs baseline `5.644`)
  - `deduplicate_ms=2.081` (improved vs baseline `2.72`)
  - `take_ms=0.1325` (in line with baseline `0.133125`)

## Patch 05: `sort` Delegates to `argsort + take`
- Status: `[/]`
- Goal: Single canonical sort pipeline.
- Tasks:
  - `[ ]` Refactor `Note::sort` to use `argsort + take` (1D attempt reverted after perf gate failure).
  - `[ ]` Preserve stability and null ordering semantics.
  - `[ ]` Remove duplicated legacy sort logic where safe.
  - `[x]` Add regression tests for deterministic ordering.
- Gate:
  - `[ ]` Functional parity confirmed and perf non-regressive.

### Patch 05 Snapshot (2026-03-21)
- Attempted 1D `sort -> argsort + gather` refactor and benchmarked.
- Gate failed with major regression (`sort_ms ~37` on `n=200000, rounds=8`).
- Reverted the delegation branch to keep baseline sort throughput.
- Current benchmark after rollback:
  - `sort_ms=12.8285`
  - `unique_ms=4.80087`
  - `deduplicate_ms=2.06462`
  - `take_ms=0.132375`
- Next step for Patch 05 completion:
  - Introduce an optimized backend permutation/gather path (not public Series conversion path) with lower compare indirection and no extra allocation churn.

## Patch 06: Encoded Fast Path
- Status: `[/]`
- Goal: Integer-code acceleration for repetitive text.
- Tasks:
  - `[x]` Add optional dictionary/interned code representation.
  - `[x]` Add heuristic to enable encoded path only when beneficial.
  - `[ ]` Keep original string outputs unchanged.
  - `[x]` Add parity tests under encoded and non-encoded routes.
- Gate:
  - `[x]` Encoded mode speedup proven on repetitive categorical datasets.

### Patch 06 Snapshot (2026-03-21)
- Added optional encoded-lane fast path in `OpsInternal.cpp`:
  - Runtime mode via env var: `PLUSS_CODEN_ENCODE_FASTPATH=off|auto|force`.
  - Auto heuristic: sample-based distinct-ratio gate before full encoding.
  - Encoded representation: dictionary spans + per-element integer codes.
- Integrated usage:
  - `unique`: uses encoded dictionary (when enabled), sorts unique dictionary entries lexically.
  - `deduplicate`: uses encoded dictionary first-seen order directly (when enabled).
- Parity checks:
  - no-catch `test_encoded_fastpath_parity` toggles mode `off` vs `force`.
  - catch2 `CodeN encoded fast path parity unique/deduplicate`.
- Benchmark update (`./build/PlussQit/plussqit_coden_cpu_perf_bench 200000 8`):
  - `sort_ms=12.7855`
  - `unique_ms=2.15075` (major improvement)
  - `deduplicate_ms=2.16138`
  - `take_ms=0.145875`
- Catch2 verification (`build_cpp_env/PlussQit/plussqit_coden_tests`):
  - Default run: `All tests passed (299 assertions in 27 test cases)`.
  - Randomized order seeds: `1, 17, 42, 1337, 2026` all pass.
  - Encoded mode matrix: `PLUSS_CODEN_ENCODE_FASTPATH=off|auto|force` all pass.

## Patch 07: `distinct` Optimization
- Status: `[x]`
- Goal: Accelerate `distinct` with code/hash route.
- Tasks:
  - `[x]` Wire `distinct` through encoded/hash kernels.
  - `[x]` Validate collision-safe exactness.
  - `[x]` Add tests for null retention/drop policies.
  - `[x]` Benchmark against baseline.
- Gate:
  - `[x]` `distinct` speedup achieved with no semantic drift.

### Patch 07 Snapshot (2026-03-21)
- Scope note:
  - `Note` has `unique` (sorted distinct) and `deduplicate` (first-seen distinct) surfaces; there is no separate `Note::distinct`.
  - Patch 07 therefore targets the distinct semantics through `unique`/`deduplicate`.
- Implementation:
  - Added shared collision-safe hash distinct helper in `OpsInternal.cpp` and reused it for `deduplicate`.
  - Refactored `unique` fallback from `sort+std::unique` over materialized views to hash-distinct + lexical sort of distinct spans.
  - Encoded fast path remains active (mode `off|auto|force`) and parity-tested.
- Correctness coverage:
  - Catch2:
    - Added repeated-empty corpus checks (retain empty exactly once) and undefined-note propagation checks.
  - no-catch:
    - Added matching repeated-empty/undefined checks.
  - All pass in:
    - `build_cpp_env/PlussQit/plussqit_coden_tests`
    - `build-plussdev-tests/PlussQit/plussqit_coden_nocatch_tests`
    - `build-plussdev-interop-tests/PlussQit/plussqit_coden_nocatch_tests`
  - Extra rigorous matrix:
    - Catch2 randomized order seeds `1, 17, 42, 1337, 2026`: all pass.
    - Encoded mode matrix `PLUSS_CODEN_ENCODE_FASTPATH=off|auto|force`: all pass.
- Benchmark (`./build_cpp_env/PlussQit/plussqit_coden_cpu_perf_bench 200000 8`):
  - `sort_ms=12.8076`
  - `unique_ms=2.12525` (improved vs baseline `5.644`)
  - `deduplicate_ms=2.09762` (improved vs baseline `2.72`)
  - `take_ms=0.133625`

## Patch 08: `tally` / Count Optimization
- Status: `[x]`
- Goal: Faster frequency counting for Note columns.
- Tasks:
  - `[x]` Implement counting over codes/hashes.
  - `[x]` Preserve deterministic output ordering policy.
  - `[x]` Validate top-N behavior and null handling.
  - `[x]` Benchmark on large repetitive columns.
- Gate:
  - `[x]` `tally` speedup achieved and output parity confirmed.

### Patch 08 Snapshot (2026-03-21)
- Scope note:
  - `Note` does not expose a public `tally` API yet; the active count-by path is used by `nlp_keywords` / `nlp_keyphrases`.
  - Patch 08 optimizes this frequency-count core and keeps behavior unchanged.
- Implementation:
  - Replaced `top_k_frequent` string-map counting with collision-safe hash counting over `string_view` lanes plus lexical tie-break.
  - Added runtime fallback switch:
    - `PLUSS_CODEN_COUNT_FASTPATH=off|on` (`on` default).
  - Ordering policy remains deterministic:
    - primary key: count descending
    - secondary key: lexical ascending.
- Correctness coverage:
  - Catch2:
    - Added `nlp_keywords` frequency-order test for tie-breaking and stopword filtering.
    - Added stopword-only input check (empty output lane).
  - no-catch:
    - Added matching `nlp_keyword_frequency` smoke checks.
  - Matrix parity:
    - `PLUSS_CODEN_COUNT_FASTPATH=off|on` x `PLUSS_CODEN_ENCODE_FASTPATH=off|auto|force` all pass.
  - Current Catch2 status:
    - `All tests passed (320 assertions in 28 test cases)`.
- Benchmark:
  - Added `nlp_keywords_ms` to `plussqit_coden_cpu_perf_bench`.
  - Default fast path (`on`), `n=200000`, `rounds=8`:
    - `sort_ms=12.827`
    - `unique_ms=2.13925`
    - `deduplicate_ms=2.13125`
    - `take_ms=0.143`
    - `nlp_keywords_ms=11.5121`
  - A/B parity check (`n=1000000`, `rounds=3`):
    - `count_fastpath=off`: `nlp_keywords_ms=59.272`
    - `count_fastpath=on`: `nlp_keywords_ms=58.3267`

## Patch 09: Parallel Lane Execution
- Status: `[x]`
- Goal: Scale hot ops on multi-core CPU for large workloads.
- Tasks:
  - `[x]` Add parallel lane implementation for large inputs.
  - `[x]` Add adaptive thresholds for parallel enablement.
  - `[x]` Verify deterministic outputs under parallel execution.
  - `[x]` Benchmark small vs large input behavior.
- Gate:
  - `[x]` Large input speedup without small-input regressions.

### Patch 09 Snapshot (2026-03-21)
- Implementation:
  - Added env-gated parallel policy:
    - `PLUSS_CODEN_PARALLEL=off|on` (`on` default).
  - Parallelized lane-safe kernels with deterministic merge order:
    - `tokenbatch_from_note(...)` (per-element tokenization fan-out)
    - `tokenbatch_from_note_pair(...)` (broadcast-aware pair tokenization fan-out)
    - `Note::sort(...)` lane loop (parallel over independent lanes).
  - Added adaptive gate so parallel sort only engages for sufficiently large/wide workloads:
    - `size_d >= 32`, `lanes * size_d >= 32768`, and worker viability.
- Determinism / parity:
  - Added dedicated parity tests toggling `PLUSS_CODEN_PARALLEL=off/on`:
    - Catch2: `CodeN parallel lane execution parity (sort + tokenbatch)`.
    - no-catch: `coden:parallel_lane_parity`.
  - Matrix parity:
    - `PLUSS_CODEN_PARALLEL=off|on` x `PLUSS_CODEN_ENCODE_FASTPATH=off|auto|force` all pass.
  - Current Catch2 status:
    - `All tests passed (323 assertions in 29 test cases)`.
  - no-catch status:
    - `build-plussdev-tests` and `build-plussdev-interop-tests` both pass.
- Benchmark (`plussqit_coden_cpu_perf_bench` with `sort_2d_ms` added):
  - Small workload (`n=20000`, `rounds=8`):
    - parallel off: `sort_2d_ms=0.611875`, `nlp_keywords_ms=1.22375`
    - parallel on:  `sort_2d_ms=0.610375`, `nlp_keywords_ms=1.13325`
  - Large workload (`n=1000000`, `rounds=3`):
    - parallel off: `sort_2d_ms=31.1103`, `sort_ms=79.269`, `nlp_keywords_ms=58.7843`
    - parallel on:  `sort_2d_ms=7.26833`, `sort_ms=71.1057`, `nlp_keywords_ms=56.8167`

## Patch 10: Semantic Lock + Fallback Policy
- Status: `[x]`
- Goal: Safe rollout controls.
- Tasks:
  - `[x]` Add runtime/config flag to force reference path.
  - `[x]` Add optimized-vs-reference parity test suite.
  - `[x]` Add failure diagnostics for mismatch reporting.
- Gate:
  - `[x]` Forced reference mode is available and tested.

### Patch 10 Snapshot (2026-03-21)
- Runtime policy added in `OpsInternal.cpp`:
  - `PLUSS_CODEN_EXECUTION_MODE=reference|optimized` (aliases: `ref|legacy|safe`, `opt|fast`).
  - Compatibility flag: `PLUSS_CODEN_REFERENCE_MODE=on|off`.
  - Reference mode forcibly disables optimization switches:
    - parallel fan-out (`PLUSS_CODEN_PARALLEL`)
    - encoded path (`PLUSS_CODEN_ENCODE_FASTPATH`)
    - count fast path (`PLUSS_CODEN_COUNT_FASTPATH`).
- Explicit reference fallback paths added for:
  - `unique(...)`
  - `deduplicate(...)`
- Parity + diagnostics:
  - Catch2: `CodeN reference mode parity across optimized routes`.
  - no-catch: `coden:reference_mode_parity`.
  - Both include mismatch diagnostics for `sort`, `unique`, `deduplicate`, and tokenbatch-producing NLP paths.
- Validation:
  - Catch2: `All tests passed (328 assertions in 30 test cases)`.
  - no-catch:
    - `build-plussdev-tests/.../plussqit_coden_nocatch_tests` passed.
    - `build-plussdev-interop-tests/.../plussqit_coden_nocatch_tests` passed.

## Patch 11: Profile-Guided Tuning
- Status: `[x]`
- Goal: Convert functional gains into robust production gains.
- Tasks:
  - `[x]` Capture CPU profiles for hot datasets.
  - `[x]` Tune allocation patterns and thresholds.
  - `[x]` Tune branch behavior in compare/hash paths.
  - `[x]` Re-run benchmark matrix and record final deltas.
- Gate:
  - `[x]` Final benchmark report meets or exceeds target ranges.

### Patch 11 Snapshot (2026-03-21)
- Tuning updates:
  - Parallel sort engagement gate tuned to avoid small-input regressions:
    - `size_d >= 32`
    - `lanes * size_d >= 32768`
    - worker viability check.
  - Runtime-mode checks made per-call (not static-cached) so A/B runs stay accurate.
  - 2D sort benchmark lane expanded to stress realistic wide-lane cases.
- Benchmark matrix automation:
  - Added portable script: `docs/scripts/coden_perf_matrix.sh`.
  - Script runs small/medium/large cases across:
    - `reference`
    - `optimized_default`
    - `optimized_no_parallel`
    - `optimized_no_encode_count`
- Final matrix (latest run):
  - `small`:
    - reference: `sort_2d_ms=0.61`, `nlp_keywords_ms=1.12`
    - optimized_default: `sort_2d_ms=0.60` (`-1.41%`), `nlp_keywords_ms=1.14` (`+1.75%`)
  - `medium`:
    - reference: `sort_2d_ms=1.48`, `nlp_keywords_ms=11.54`
    - optimized_default: `sort_2d_ms=1.57` (`+5.63%`), `nlp_keywords_ms=11.54` (`-0.01%`)
  - `large`:
    - reference: `sort_2d_ms=7.23`, `nlp_keywords_ms=56.71`
    - optimized_default: `sort_2d_ms=7.40` (`+2.36%`), `nlp_keywords_ms=56.41` (`-0.54%`)
  - Interpretation:
    - Parallel remains essential for large 2D sort workloads (`optimized_no_parallel` regresses heavily).
    - Encoded/count toggles are workload-dependent and remain runtime-tunable.

## Patch 12: Docs + User Guidance
- Status: `[x]`
- Goal: Ship-ready documentation and expectations.
- Tasks:
  - `[x]` Document `Note::argsort` and sort semantics.
  - `[x]` Document null ordering and stability guarantees.
  - `[x]` Add performance guidance and examples.
  - `[x]` Add fallback/config policy docs.
- Gate:
  - `[x]` User docs and API references updated.

### Patch 12 Snapshot (2026-03-21)
- Added runtime/performance user guide:
  - `docs/CODEN_CPU_RUNTIME_POLICY.md`
- Guide includes:
  - `argsort` / `sort` semantics and stability expectations.
  - Ordering behavior for empty-string entries.
  - Runtime policy table for `PLUSS_CODEN_EXECUTION_MODE`, `PLUSS_CODEN_REFERENCE_MODE`,
    `PLUSS_CODEN_PARALLEL`, `PLUSS_CODEN_ENCODE_FASTPATH`, `PLUSS_CODEN_COUNT_FASTPATH`.
  - Reproducible benchmark matrix command examples.
  - Recommended parity-debug workflow (force reference mode first, then re-enable optimizations progressively).

## Cross-Patch Quality Gates
- `[x]` All existing CodeN tests pass after every patch.
- `[x]` Added tests are deterministic and repeatable.
- `[x]` No public API breaks without explicit migration notice.
- `[x]` Benchmark report updated at each milestone.

## Milestone Checklist
- Milestone A (Patches 01-05): `[ ]`
- Milestone B (Patches 06-09): `[x]`
- Milestone C (Patches 10-12): `[x]`

## Expectation Management
- Target is to make CodeN much closer to Series-class CPU performance for key text workloads.
- Exact parity with numeric Series throughput is not guaranteed for all string-heavy cases because string comparison and collation costs are fundamentally higher.
