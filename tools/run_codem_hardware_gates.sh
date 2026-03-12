#!/usr/bin/env bash
set -euo pipefail

BACKEND="mps"
CONDA_ENV="cpp_env"
BUILD_DIR="build_cpp_env"
OUT_ROOT="outputs"
SEED="1337"
CONDA_BIN="${CONDA_BIN:-}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --backend)
      BACKEND="${2:-}"
      shift 2
      ;;
    --env)
      CONDA_ENV="${2:-}"
      shift 2
      ;;
    --build-dir)
      BUILD_DIR="${2:-}"
      shift 2
      ;;
    --out-root)
      OUT_ROOT="${2:-}"
      shift 2
      ;;
    --seed)
      SEED="${2:-}"
      shift 2
      ;;
    --conda-bin)
      CONDA_BIN="${2:-}"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      echo "Usage: $0 [--backend mps|cuda] [--env cpp_env] [--build-dir build_cpp_env] [--out-root outputs] [--seed 1337] [--conda-bin /abs/path/conda]" >&2
      exit 2
      ;;
  esac
done

if [[ -z "${CONDA_BIN}" ]]; then
  if command -v conda >/dev/null 2>&1; then
    CONDA_BIN="$(command -v conda)"
  elif [[ -x "/Users/yawasante/software/miniconda3/bin/conda" ]]; then
    CONDA_BIN="/Users/yawasante/software/miniconda3/bin/conda"
  else
    echo "Could not resolve conda binary. Use --conda-bin." >&2
    exit 2
  fi
fi

BIN="${BUILD_DIR}/PlussQit/plussqit_codem_mps_selection_tests"
if [[ ! -x "${BIN}" ]]; then
  echo "Test binary not found or not executable: ${BIN}" >&2
  exit 2
fi

TS="$(date +%Y%m%d_%H%M%S)"
OUTDIR="${OUT_ROOT}/${BACKEND}_mega_gate_${TS}"
mkdir -p "${OUTDIR}"
BACKEND_UPPER="$(printf "%s" "${BACKEND}" | tr '[:lower:]' '[:upper:]')"

echo "run_name|exit_code|summary" > "${OUTDIR}/summary.tsv"
{
  echo "CodeM Hardware Gate Sweep - ${BACKEND_UPPER}"
  echo
  echo "- Date: $(date)"
  echo "- Conda env: ${CONDA_ENV}"
  echo "- Test binary: ${BIN}"
  echo
} > "${OUTDIR}/one_page.md"

run_gate() {
  local name="$1"
  local filter="$2"
  shift 2
  local logfile="${OUTDIR}/${name}.log"
  set +e
  env "$@" "${CONDA_BIN}" run -n "${CONDA_ENV}" "${BIN}" "${filter}" --rng-seed "${SEED}" --order lex > "${logfile}" 2>&1
  local rc=$?
  set -e

  local summary
  summary="$(rg -m1 "All tests passed|test cases|No tests ran" "${logfile}" || true)"
  if [[ -z "${summary}" ]]; then
    summary="(no Catch2 summary line found)"
  fi
  echo "${name}|${rc}|${summary}" >> "${OUTDIR}/summary.tsv"
}

case "${BACKEND}" in
  mps)
    run_gate gate01_default_mps "[mps]"
    run_gate gate02_strict_no_bridge "[strict]" \
      PLUSS_MPS_ALLOW_CPU_FALLBACK=0 \
      PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE=0
    run_gate gate03_native_only_linearalg "[mps][linearalg][native]" \
      PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=1 \
      PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE=0 \
      PLUSS_MPS_LINALG_SOLVER_ROUTE=native \
      PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD=1
    run_gate gate04_focus_take_put "[mps][take],[mps][put]"
    run_gate gate05_focus_qr_svd_bridge "[mps][linearalg][bridge][qr][svd]" \
      PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=0 \
      PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE=1
    run_gate gate06_focus_cache_coherency "[mps][linearalg][bridge][cache][coherency]" \
      PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=0 \
      PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE=1
    run_gate gate07_focus_sort_native_strict "[mps][sort][native][strict]" \
      PLUSS_MPS_NATIVE_SORT_SELECTION=1 \
      PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT=1 \
      PLUSS_MPS_ALLOW_CPU_FALLBACK=0
    run_gate gate08_focus_strict_native_svd "[mps][linearalg][native][svd][strict]" \
      PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=1 \
      PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE=0 \
      PLUSS_MPS_LINALG_SOLVER_ROUTE=native \
      PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD=1
    run_gate gate09_focus_selection_topk_optin "[mps][topk],[mps][kthvalue]" \
      PLUSS_MPS_NATIVE_TOPK_SELECTION=1
    run_gate gate10_focus_native_telemetry "[mps][linearalg][native][telemetry]" \
      PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=1 \
      PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD=1
    run_gate gate11_focus_take_put_native_strict "[mps][take][native][strict],[mps][put][native][strict]" \
      PLUSS_MPS_ALLOW_CPU_FALLBACK=0 \
      PLUSS_MPS_NATIVE_TAKE_PUT=1
    run_gate gate12_focus_take_native_policy "[mps][take][native][strict][policy]" \
      PLUSS_MPS_ALLOW_CPU_FALLBACK=0 \
      PLUSS_MPS_NATIVE_TAKE_PUT=1
    run_gate gate13_focus_put_native_policy "[mps][put][native][strict][policy]" \
      PLUSS_MPS_ALLOW_CPU_FALLBACK=0 \
      PLUSS_MPS_NATIVE_TAKE_PUT=1
    run_gate gate14_torch_family_parity "[mps][parity][torch]"
    run_gate gate15_torch_family_parity_strict "[mps][parity][torch][strict]" \
      PLUSS_MPS_ALLOW_CPU_FALLBACK=0 \
      PLUSS_MPS_NATIVE_TAKE_PUT=1 \
      PLUSS_MPS_NATIVE_SORT_SELECTION=1 \
      PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT=1 \
      PLUSS_MPS_NATIVE_TOPK_SELECTION=1 \
      PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=1 \
      PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE=0 \
      PLUSS_MPS_LINALG_SOLVER_ROUTE=native \
      PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD=1
    ;;
  cuda)
    run_gate gate01_cuda_bootstrap_backend "[cuda][bootstrap][backend]"
    run_gate gate02_cuda_bootstrap_runtime "[cuda][bootstrap][runtime]"
    run_gate gate03_cuda_bootstrap_policy "[cuda][bootstrap][policy]"
    run_gate gate04_cuda_baseline_runtime "[cuda][baseline][runtime]"
    run_gate gate05_cuda_selection_runtime "[cuda][selection][runtime]"
    run_gate gate06_cuda_selection_policy_strict "[cuda][selection][policy]" \
      PLUSS_CUDA_NATIVE_BASELINE=1 \
      PLUSS_CUDA_NATIVE_SELECTION=1 \
      PLUSS_CUDA_NATIVE_SELECTION_STRICT=1
    run_gate gate07_cuda_linalg_runtime "[cuda][linalg][runtime]"
    run_gate gate08_cuda_linalg_policy_strict "[cuda][linalg][policy]" \
      PLUSS_CUDA_NATIVE_BASELINE=1 \
      PLUSS_CUDA_NATIVE_LINALG=1 \
      PLUSS_CUDA_NATIVE_LINALG_STRICT=1
    run_gate gate09_cuda_bridge_runtime "[cuda][bridge][runtime],[cuda][bridge][selection],[cuda][bridge][linalg]"
    run_gate gate10_cuda_bridge_policy "[cuda][bridge][policy]" \
      PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
      PLUSS_CUDA_NATIVE_BASELINE=1 \
      PLUSS_CUDA_NATIVE_LINALG=1 \
      PLUSS_CUDA_NATIVE_LINALG_STRICT=1
    run_gate gate11_cuda_indexing_runtime "[cuda][indexing][runtime]"
    run_gate gate12_cuda_indexing_policy "[cuda][indexing][policy]"
    run_gate gate13_cuda_indexing_policy_strict "[cuda][bridge][policy][indexing]" \
      PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
      PLUSS_CUDA_NATIVE_BASELINE=1 \
      PLUSS_CUDA_NATIVE_INDEXING=1 \
      PLUSS_CUDA_NATIVE_INDEXING_STRICT=1
    run_gate gate14_cuda_bridge_indexing_runtime "[cuda][bridge][indexing]" \
      PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
      PLUSS_CUDA_NATIVE_INDEXING=0
    run_gate gate15_cuda_matmul_runtime "[cuda][matmul][runtime]"
    run_gate gate16_cuda_matmul_policy_strict "[cuda][bridge][policy][matmul]" \
      PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
      PLUSS_CUDA_NATIVE_BASELINE=1 \
      PLUSS_CUDA_NATIVE_MATMUL=1 \
      PLUSS_CUDA_NATIVE_MATMUL_STRICT=1
    run_gate gate17_cuda_scatter_runtime "[cuda][scatter][runtime]"
    run_gate gate18_cuda_scatter_policy_strict "[cuda][bridge][policy][scatter]" \
      PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
      PLUSS_CUDA_NATIVE_BASELINE=1 \
      PLUSS_CUDA_NATIVE_SCATTER_GATHER=1 \
      PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT=1
    run_gate gate19_cuda_bridge_matmul_scatter "[cuda][bridge][matmul],[cuda][bridge][scatter]"
    run_gate gate20_accelerator_backend_info "[linearalg][backend][accelerators]"
    run_gate gate21_cuda_runtime_inventory "[cuda][bootstrap][inventory],[cuda][providers][runtime]"
    run_gate gate22_cuda_provider_backend_info "[cuda][providers][backend]"
    run_gate gate23_cuda_torch_family_parity_bridge "[cuda][parity][bridge]" \
      PLUSS_CUDA_ALLOW_CPU_BRIDGE=1
    run_gate gate24_cuda_torch_family_parity_bridge_stress "[cuda][parity][bridge][stress]" \
      PLUSS_CUDA_ALLOW_CPU_BRIDGE=1
    run_gate gate25_cuda_torch_family_parity_strict "[cuda][parity][policy]" \
      PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
      PLUSS_CUDA_NATIVE_BASELINE=1 \
      PLUSS_CUDA_NATIVE_SELECTION=1 \
      PLUSS_CUDA_NATIVE_SELECTION_STRICT=1 \
      PLUSS_CUDA_NATIVE_LINALG=1 \
      PLUSS_CUDA_NATIVE_LINALG_STRICT=1 \
      PLUSS_CUDA_NATIVE_INDEXING=1 \
      PLUSS_CUDA_NATIVE_INDEXING_STRICT=1 \
      PLUSS_CUDA_NATIVE_MATMUL=1 \
      PLUSS_CUDA_NATIVE_MATMUL_STRICT=1 \
      PLUSS_CUDA_NATIVE_SCATTER_GATHER=1 \
      PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT=1
    ;;
  *)
    echo "Unsupported backend '${BACKEND}'. Use mps or cuda." >&2
    exit 2
    ;;
esac

PASS_COUNT="$(awk -F'|' 'NR>1 && $2==0 {c++} END{print c+0}' "${OUTDIR}/summary.tsv")"
TOTAL_COUNT="$(awk 'END{print NR-1}' "${OUTDIR}/summary.tsv")"

{
  echo "## Sweep Result"
  echo "- Total gates: ${TOTAL_COUNT}"
  echo "- Passing gates: ${PASS_COUNT}"
  echo
  echo "## Gate Table"
  echo "| Gate | Exit | Catch2 Summary |"
  echo "|---|---:|---|"
  awk -F'|' 'NR>1{printf "| %s | %s | %s |\n", $1, $2, $3}' "${OUTDIR}/summary.tsv"
} >> "${OUTDIR}/one_page.md"

OUTDIR_ABS="$(cd "${OUTDIR}" && pwd)"
LATEST_PTR="${OUT_ROOT}/latest_${BACKEND}_mega_gate_path.txt"
echo "${OUTDIR_ABS}" > "${LATEST_PTR}"
ln -sfn "$(basename "${OUTDIR}")" "${OUT_ROOT}/latest_${BACKEND}_mega_gate"

echo "${OUTDIR}"
