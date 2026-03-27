#!/usr/bin/env bash
set -euo pipefail

BENCH_BIN="${1:-/Users/yawasante/software/cxx/PlussDev/build_cpp_env/PlussQit/plussqit_coden_cpu_perf_bench}"

if [[ ! -x "$BENCH_BIN" ]]; then
  echo "error: benchmark binary not executable: $BENCH_BIN" >&2
  exit 1
fi

run_bench() {
  local env_spec="$1"
  local n="$2"
  local rounds="$3"
  if [[ -n "$env_spec" ]]; then
    env $env_spec "$BENCH_BIN" "$n" "$rounds"
  else
    "$BENCH_BIN" "$n" "$rounds"
  fi
}

metric() {
  local key="$1"
  awk -F= -v k="$key" '$1==k {print $2}'
}

fmt2() {
  awk -v x="$1" 'BEGIN{printf "%.2f", x}'
}

delta_pct() {
  local base="$1"
  local cur="$2"
  awk -v b="$base" -v c="$cur" 'BEGIN{ if (b == 0) { printf "n/a"; } else { printf "%.2f%%", ((c-b)/b)*100.0; } }'
}

declare -a CASES=(
  "small:20000:8"
  "medium:200000:8"
  "large:1000000:3"
)

REF_PROFILE_ENV="PLUSS_CODEN_EXECUTION_MODE=reference PLUSS_CODEN_PARALLEL=on PLUSS_CODEN_ENCODE_FASTPATH=force PLUSS_CODEN_COUNT_FASTPATH=on"

declare -a OPT_PROFILES=(
  "optimized_default:"
  "optimized_no_parallel:PLUSS_CODEN_PARALLEL=off"
  "optimized_no_encode_count:PLUSS_CODEN_ENCODE_FASTPATH=off PLUSS_CODEN_COUNT_FASTPATH=off"
)

echo "| case | profile | sort_ms | sort_2d_ms | nlp_keywords_ms | d_sort_2d_vs_ref | d_keywords_vs_ref |"
echo "|---|---:|---:|---:|---:|---:|---:|"

for case_spec in "${CASES[@]}"; do
  IFS=":" read -r case_name n rounds <<< "$case_spec"
  ref_out="$(run_bench "$REF_PROFILE_ENV" "$n" "$rounds")"
  ref_sort_ms="$(printf '%s\n' "$ref_out" | metric sort_ms)"
  ref_sort2d_ms="$(printf '%s\n' "$ref_out" | metric sort_2d_ms)"
  ref_kw_ms="$(printf '%s\n' "$ref_out" | metric nlp_keywords_ms)"
  printf "| %s | %s | %s | %s | %s | %s | %s |\n" \
    "$case_name" "reference" "$(fmt2 "$ref_sort_ms")" "$(fmt2 "$ref_sort2d_ms")" "$(fmt2 "$ref_kw_ms")" "0.00%" "0.00%"

  for profile_spec in "${OPT_PROFILES[@]}"; do
    IFS=":" read -r profile_name env_spec <<< "$profile_spec"
    out="$(run_bench "$env_spec" "$n" "$rounds")"

    sort_ms="$(printf '%s\n' "$out" | metric sort_ms)"
    sort2d_ms="$(printf '%s\n' "$out" | metric sort_2d_ms)"
    kw_ms="$(printf '%s\n' "$out" | metric nlp_keywords_ms)"

    d_sort2d="$(delta_pct "$ref_sort2d_ms" "$sort2d_ms")"
    d_kw="$(delta_pct "$ref_kw_ms" "$kw_ms")"
    printf "| %s | %s | %s | %s | %s | %s | %s |\n" \
      "$case_name" "$profile_name" "$(fmt2 "$sort_ms")" "$(fmt2 "$sort2d_ms")" "$(fmt2 "$kw_ms")" "$d_sort2d" "$d_kw"
  done
done
