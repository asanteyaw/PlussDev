# CodeN CPU Runtime Policy

This guide documents the runtime controls and expected behavior for CodeN CPU optimized paths.

## Sorting And Ordering Semantics

### `Note::sort(dim)`
- Purpose:
  - Returns values sorted lexicographically along `dim`.
- Inputs:
  - `dim`: target dimension (`-1` means last dimension).
- Output:
  - `Note` with values sorted along the selected dimension.
- Behavior:
  - Deterministic ordering.
  - Stable ordering for equal values in the current implementation.

### `Note::argsort(descending, stable)` (interop-enabled builds)
- Purpose:
  - Returns permutation indices that sort a 1D/scalar `Note`.
- Inputs:
  - `descending`: `false` for ascending, `true` for descending.
  - `stable`: `true` for stable sort, `false` for unstable sort.
- Output:
  - `Series<int64>` permutation indices.
- Notes:
  - Available when `CODEN_ENABLE_SERIES_INTEROP` is enabled.
  - Current support is 1D/scalar `Note`.

### Empty Strings / Missing-Text Representation
- CodeN `Note` ordering compares text lexicographically at the byte level.
- Empty strings therefore sort before non-empty values in ascending order.

## Runtime Flags

### `PLUSS_CODEN_EXECUTION_MODE`
- Values:
  - `optimized` (default)
  - `reference`
- Aliases:
  - reference: `ref`, `legacy`, `safe`
  - optimized: `opt`, `fast`
- Effect:
  - `reference` forces conservative/reference paths for guarded optimized operations.

### `PLUSS_CODEN_REFERENCE_MODE`
- Values:
  - `on|off`, `true|false`, `1|0`, `yes|no`
- Effect:
  - Compatibility boolean switch for forcing reference mode.

### `PLUSS_CODEN_PARALLEL`
- Values:
  - `on|off`
- Effect:
  - Enables/disables parallel lane execution on supported kernels.
- Note:
  - Ignored when reference mode is forced.

### `PLUSS_CODEN_ENCODE_FASTPATH`
- Values:
  - `off|auto|force`
- Effect:
  - Controls dictionary/code-based accelerated path for repeated text workloads.
- Note:
  - Ignored when reference mode is forced.

### `PLUSS_CODEN_COUNT_FASTPATH`
- Values:
  - `on|off`
- Effect:
  - Controls optimized frequency-count path used by keyword-count style routines.
- Note:
  - Ignored when reference mode is forced.

## Recommended Debug Workflow

1. Reproduce in reference mode:
   - `PLUSS_CODEN_EXECUTION_MODE=reference`
2. If reference output is correct, switch to optimized:
   - `PLUSS_CODEN_EXECUTION_MODE=optimized`
3. Narrow cause by toggling one accelerator at a time:
   - `PLUSS_CODEN_PARALLEL`
   - `PLUSS_CODEN_ENCODE_FASTPATH`
   - `PLUSS_CODEN_COUNT_FASTPATH`

## Benchmark Matrix Workflow

Use:

```bash
./docs/scripts/coden_perf_matrix.sh
```

Or with explicit bench binary:

```bash
./docs/scripts/coden_perf_matrix.sh /path/to/plussqit_coden_cpu_perf_bench
```

The script reports:
- `sort_ms`
- `sort_2d_ms`
- `nlp_keywords_ms`
- percentage delta versus reference mode for each case/profile.

