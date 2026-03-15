#!/usr/bin/env bash
set -euo pipefail

CONDA_ENV="cpp_env"
BUILD_DIR="build_cuda_cluster"
OUT_ROOT="outputs"
SEED="1337"
STRESS_REPS="3"
CONDA_BIN="${CONDA_BIN:-}"

while [[ $# -gt 0 ]]; do
  case "$1" in
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
    --stress-reps)
      STRESS_REPS="${2:-}"
      shift 2
      ;;
    --conda-bin)
      CONDA_BIN="${2:-}"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      echo "Usage: $0 [--env cpp_env] [--build-dir build_cuda_cluster] [--out-root outputs] [--seed 1337] [--stress-reps 3] [--conda-bin /abs/path/conda]" >&2
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

if ! [[ "${STRESS_REPS}" =~ ^[0-9]+$ ]]; then
  echo "--stress-reps must be a non-negative integer" >&2
  exit 2
fi

TS="$(date +%Y%m%d_%H%M%S)"
OUTDIR="${OUT_ROOT}/cuda_cluster_hardware_${TS}"
mkdir -p "${OUTDIR}"
SUMMARY_TSV="${OUTDIR}/summary.tsv"
printf "step|exit_code|notes\n" > "${SUMMARY_TSV}"

run_optional() {
  local name="$1"
  shift
  local log="${OUTDIR}/${name}.log"
  set +e
  "$@" >"${log}" 2>&1
  local rc=$?
  set -e
  printf "%s|%s|%s\n" "${name}" "${rc}" "optional" >> "${SUMMARY_TSV}"
}

run_required() {
  local name="$1"
  shift
  local log="${OUTDIR}/${name}.log"
  set +e
  "$@" >"${log}" 2>&1
  local rc=$?
  set -e
  printf "%s|%s|%s\n" "${name}" "${rc}" "required" >> "${SUMMARY_TSV}"
  if [[ ${rc} -ne 0 ]]; then
    echo "Required step failed: ${name}. See ${log}" >&2
    exit ${rc}
  fi
}

cpu_jobs() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
    return
  fi
  if command -v sysctl >/dev/null 2>&1; then
    sysctl -n hw.ncpu 2>/dev/null || echo 8
    return
  fi
  echo 8
}

run_optional host_uname uname -a
run_optional host_date date
run_optional cmake_version cmake --version
run_optional conda_info "${CONDA_BIN}" info --envs
run_optional nvidia_smi_path which nvidia-smi
run_optional nvcc_path which nvcc
run_optional nvidia_smi_list nvidia-smi -L
run_optional nvidia_smi_query nvidia-smi --query-gpu=name,driver_version,cuda_version,compute_cap,memory.total --format=csv,noheader
run_optional nvcc_version nvcc --version

run_required configure_cmake \
  "${CONDA_BIN}" run -n "${CONDA_ENV}" \
  cmake -S . -B "${BUILD_DIR}" \
    -DPLUSSQIT_BUILD_CODEM=ON \
    -DPLUSSQIT_BUILD_TESTS=ON \
    -DPLUSSQIT_ENABLE_CUDA=ON

run_required build_cuda_tests \
  "${CONDA_BIN}" run -n "${CONDA_ENV}" \
  cmake --build "${BUILD_DIR}" --target plussqit_codem_cuda_gate_tests -j "$(cpu_jobs)"

TEST_BIN="${BUILD_DIR}/PlussQit/plussqit_codem_cuda_gate_tests"
if [[ ! -x "${TEST_BIN}" ]]; then
  echo "Expected test binary missing: ${TEST_BIN}" >&2
  exit 2
fi

run_required cuda_provider_focus \
  "${CONDA_BIN}" run -n "${CONDA_ENV}" \
  "${TEST_BIN}" "[cuda][providers]" --rng-seed "${SEED}" --order lex

GATE_STDOUT_LOG="${OUTDIR}/cuda_mega_gate.log"
set +e
GATE_STDOUT="$(tools/run_codem_hardware_gates.sh --backend cuda --env "${CONDA_ENV}" --build-dir "${BUILD_DIR}" --out-root "${OUT_ROOT}" --seed "${SEED}" --conda-bin "${CONDA_BIN}" 2>&1)"
GATE_RC=$?
set -e
printf "%s\n" "${GATE_STDOUT}" > "${GATE_STDOUT_LOG}"
printf "cuda_mega_gate|%s|required\n" "${GATE_RC}" >> "${SUMMARY_TSV}"
if [[ ${GATE_RC} -ne 0 ]]; then
  echo "CUDA mega-gate failed. See ${GATE_STDOUT_LOG}" >&2
  exit ${GATE_RC}
fi

GATE_OUT_REL="$(printf "%s\n" "${GATE_STDOUT}" | tail -n 1)"
if [[ ! -d "${GATE_OUT_REL}" ]]; then
  echo "Could not resolve CUDA gate artifact directory from output: ${GATE_OUT_REL}" >&2
  exit 2
fi

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge]" --rng-seed "$((SEED + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_MATMUL=1 PLUSS_CUDA_NATIVE_MATMUL_STRICT=1 PLUSS_CUDA_NATIVE_SCATTER_GATHER=1 PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT=1 PLUSS_CUDA_NATIVE_REDUCTION=1 PLUSS_CUDA_NATIVE_REDUCTION_STRICT=1 PLUSS_CUDA_NATIVE_ELEMENTWISE=1 PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT=1 PLUSS_CUDA_NATIVE_SCAN=1 PLUSS_CUDA_NATIVE_SCAN_STRICT=1 PLUSS_CUDA_NATIVE_MASKING=1 PLUSS_CUDA_NATIVE_MASKING_STRICT=1 PLUSS_CUDA_NATIVE_BITWISE=1 PLUSS_CUDA_NATIVE_BITWISE_STRICT=1 PLUSS_CUDA_NATIVE_COMPOSE=1 PLUSS_CUDA_NATIVE_COMPOSE_STRICT=1 PLUSS_CUDA_NATIVE_STOCHASTIC=1 PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy]" --rng-seed "$((SEED + 100 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_elementwise_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][elementwise]" --rng-seed "$((SEED + 185 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_elementwise_policy_rep${rep}" \
    env PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_ELEMENTWISE=1 PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][elementwise]" --rng-seed "$((SEED + 195 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_scan_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][scan]" --rng-seed "$((SEED + 205 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_scan_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_SCAN=1 PLUSS_CUDA_NATIVE_SCAN_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][scan]" --rng-seed "$((SEED + 215 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_activation_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_ELEMENTWISE=1 PLUSS_CUDA_NATIVE_REDUCTION=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][activation]" --rng-seed "$((SEED + 225 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_activation_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_ELEMENTWISE=1 PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT=1 PLUSS_CUDA_NATIVE_REDUCTION=1 PLUSS_CUDA_NATIVE_REDUCTION_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][activation]" --rng-seed "$((SEED + 235 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_masking_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][masking]" --rng-seed "$((SEED + 245 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_masking_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_MASKING=1 PLUSS_CUDA_NATIVE_MASKING_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][masking]" --rng-seed "$((SEED + 255 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_bitwise_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][bitwise]" --rng-seed "$((SEED + 265 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_bitwise_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_BITWISE=1 PLUSS_CUDA_NATIVE_BITWISE_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][bitwise]" --rng-seed "$((SEED + 275 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_compose_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][compose]" --rng-seed "$((SEED + 285 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_compose_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_COMPOSE=1 PLUSS_CUDA_NATIVE_COMPOSE_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][compose]" --rng-seed "$((SEED + 295 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_counting_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][counting]" --rng-seed "$((SEED + 299 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_counting_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_COUNTING=1 PLUSS_CUDA_NATIVE_COUNTING_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][counting]" --rng-seed "$((SEED + 309 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_selection_route_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][selection][route]" --rng-seed "$((SEED + 319 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_selection_route_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_SELECTION=1 PLUSS_CUDA_NATIVE_SELECTION_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][selection][route]" --rng-seed "$((SEED + 329 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_selection_route_reject_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_SELECTION=1 PLUSS_CUDA_NATIVE_SELECTION_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][selection][reject]" --rng-seed "$((SEED + 339 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_factory_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][factory]" --rng-seed "$((SEED + 349 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_factory_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_FACTORY=1 PLUSS_CUDA_NATIVE_FACTORY_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][factory]" --rng-seed "$((SEED + 359 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_factory_policy_reject_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=0 PLUSS_CUDA_NATIVE_FACTORY=1 PLUSS_CUDA_NATIVE_FACTORY_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][factory][reject]" --rng-seed "$((SEED + 369 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_stochastic_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][stochastic]" --rng-seed "$((SEED + 379 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_stochastic_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_STOCHASTIC=1 PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][stochastic]" --rng-seed "$((SEED + 389 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_linalg_batched_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][linalg][batched]" --rng-seed "$((SEED + 305 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_linalg_batched_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_LINALG=1 PLUSS_CUDA_NATIVE_LINALG_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][linalg][batched]" --rng-seed "$((SEED + 315 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_linalg_expanded_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[bridge][cuda][expanded][linalg]" --rng-seed "$((SEED + 325 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_linalg_expanded_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_LINALG=1 PLUSS_CUDA_NATIVE_LINALG_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[bridge][cuda][expanded][linalg][policy]" --rng-seed "$((SEED + 335 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_matmul_rank_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][matmul][rank]" --rng-seed "$((SEED + 345 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_matmul_rank_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_MATMUL=1 PLUSS_CUDA_NATIVE_MATMUL_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][matmul][rank]" --rng-seed "$((SEED + 355 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_reduction_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][reduction]" --rng-seed "$((SEED + 180 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_reduction_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_REDUCTION=1 PLUSS_CUDA_NATIVE_REDUCTION_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][bridge][policy][reduction]" --rng-seed "$((SEED + 190 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_torch_parity_bridge_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][parity][bridge],[cuda][parity][bridge][stress]" --rng-seed "$((SEED + 200 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_torch_parity_policy_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_SELECTION=1 PLUSS_CUDA_NATIVE_SELECTION_STRICT=1 PLUSS_CUDA_NATIVE_LINALG=1 PLUSS_CUDA_NATIVE_LINALG_STRICT=1 PLUSS_CUDA_NATIVE_INDEXING=1 PLUSS_CUDA_NATIVE_INDEXING_STRICT=1 PLUSS_CUDA_NATIVE_MATMUL=1 PLUSS_CUDA_NATIVE_MATMUL_STRICT=1 PLUSS_CUDA_NATIVE_SCATTER_GATHER=1 PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT=1 PLUSS_CUDA_NATIVE_REDUCTION=1 PLUSS_CUDA_NATIVE_REDUCTION_STRICT=1 PLUSS_CUDA_NATIVE_ELEMENTWISE=1 PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT=1 PLUSS_CUDA_NATIVE_SCAN=1 PLUSS_CUDA_NATIVE_SCAN_STRICT=1 PLUSS_CUDA_NATIVE_MASKING=1 PLUSS_CUDA_NATIVE_MASKING_STRICT=1 PLUSS_CUDA_NATIVE_BITWISE=1 PLUSS_CUDA_NATIVE_BITWISE_STRICT=1 PLUSS_CUDA_NATIVE_COMPOSE=1 PLUSS_CUDA_NATIVE_COMPOSE_STRICT=1 PLUSS_CUDA_NATIVE_STOCHASTIC=1 PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][parity][policy]" --rng-seed "$((SEED + 300 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_native_strict_matrix_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_SELECTION=1 PLUSS_CUDA_NATIVE_SELECTION_STRICT=1 PLUSS_CUDA_NATIVE_LINALG=1 PLUSS_CUDA_NATIVE_LINALG_STRICT=1 PLUSS_CUDA_NATIVE_INDEXING=1 PLUSS_CUDA_NATIVE_INDEXING_STRICT=1 PLUSS_CUDA_NATIVE_MATMUL=1 PLUSS_CUDA_NATIVE_MATMUL_STRICT=1 PLUSS_CUDA_NATIVE_SCATTER_GATHER=1 PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT=1 PLUSS_CUDA_NATIVE_REDUCTION=1 PLUSS_CUDA_NATIVE_REDUCTION_STRICT=1 PLUSS_CUDA_NATIVE_ELEMENTWISE=1 PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT=1 PLUSS_CUDA_NATIVE_SCAN=1 PLUSS_CUDA_NATIVE_SCAN_STRICT=1 PLUSS_CUDA_NATIVE_MASKING=1 PLUSS_CUDA_NATIVE_MASKING_STRICT=1 PLUSS_CUDA_NATIVE_BITWISE=1 PLUSS_CUDA_NATIVE_BITWISE_STRICT=1 PLUSS_CUDA_NATIVE_COMPOSE=1 PLUSS_CUDA_NATIVE_COMPOSE_STRICT=1 PLUSS_CUDA_NATIVE_STOCHASTIC=1 PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][parity][native][strict]" --rng-seed "$((SEED + 400 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_native_strict_soak_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_SELECTION=1 PLUSS_CUDA_NATIVE_SELECTION_STRICT=1 PLUSS_CUDA_NATIVE_LINALG=1 PLUSS_CUDA_NATIVE_LINALG_STRICT=1 PLUSS_CUDA_NATIVE_INDEXING=1 PLUSS_CUDA_NATIVE_INDEXING_STRICT=1 PLUSS_CUDA_NATIVE_MATMUL=1 PLUSS_CUDA_NATIVE_MATMUL_STRICT=1 PLUSS_CUDA_NATIVE_SCATTER_GATHER=1 PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT=1 PLUSS_CUDA_NATIVE_REDUCTION=1 PLUSS_CUDA_NATIVE_REDUCTION_STRICT=1 PLUSS_CUDA_NATIVE_ELEMENTWISE=1 PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT=1 PLUSS_CUDA_NATIVE_SCAN=1 PLUSS_CUDA_NATIVE_SCAN_STRICT=1 PLUSS_CUDA_NATIVE_MASKING=1 PLUSS_CUDA_NATIVE_MASKING_STRICT=1 PLUSS_CUDA_NATIVE_BITWISE=1 PLUSS_CUDA_NATIVE_BITWISE_STRICT=1 PLUSS_CUDA_NATIVE_COMPOSE=1 PLUSS_CUDA_NATIVE_COMPOSE_STRICT=1 PLUSS_CUDA_NATIVE_STOCHASTIC=1 PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][parity][native][soak]" --rng-seed "$((SEED + 500 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_native_expanded_rep${rep}" \
    env PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 PLUSS_CUDA_NATIVE_BASELINE=1 PLUSS_CUDA_NATIVE_SELECTION=1 PLUSS_CUDA_NATIVE_SELECTION_STRICT=1 PLUSS_CUDA_NATIVE_INDEXING=1 PLUSS_CUDA_NATIVE_INDEXING_STRICT=1 PLUSS_CUDA_NATIVE_MATMUL=1 PLUSS_CUDA_NATIVE_MATMUL_STRICT=1 PLUSS_CUDA_NATIVE_SCATTER_GATHER=1 PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT=1 PLUSS_CUDA_NATIVE_REDUCTION=1 PLUSS_CUDA_NATIVE_REDUCTION_STRICT=1 PLUSS_CUDA_NATIVE_ELEMENTWISE=1 PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT=1 PLUSS_CUDA_NATIVE_SCAN=1 PLUSS_CUDA_NATIVE_SCAN_STRICT=1 PLUSS_CUDA_NATIVE_MASKING=1 PLUSS_CUDA_NATIVE_MASKING_STRICT=1 PLUSS_CUDA_NATIVE_BITWISE=1 PLUSS_CUDA_NATIVE_BITWISE_STRICT=1 PLUSS_CUDA_NATIVE_COMPOSE=1 PLUSS_CUDA_NATIVE_COMPOSE_STRICT=1 PLUSS_CUDA_NATIVE_STOCHASTIC=1 PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT=1 \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][parity][native][expanded]" --rng-seed "$((SEED + 550 + rep))" --order lex

done

for rep in $(seq 1 "${STRESS_REPS}"); do
  run_required "stress_provider_inventory_rep${rep}" \
    "${CONDA_BIN}" run -n "${CONDA_ENV}" \
    "${TEST_BIN}" "[cuda][providers][inventory][stress]" --rng-seed "$((SEED + 600 + rep))" --order lex

done

cp "${GATE_OUT_REL}/one_page.md" "${OUTDIR}/cuda_gate_one_page.md"
cp "${GATE_OUT_REL}/summary.tsv" "${OUTDIR}/cuda_gate_summary.tsv"

REQ_PASS_COUNT="$(awk -F'|' 'NR>1 && $2==0 && $3=="required" {c++} END{print c+0}' "${SUMMARY_TSV}")"
REQ_TOTAL_COUNT="$(awk -F'|' 'NR>1 && $3=="required" {c++} END{print c+0}' "${SUMMARY_TSV}")"
OPT_PASS_COUNT="$(awk -F'|' 'NR>1 && $2==0 && $3=="optional" {c++} END{print c+0}' "${SUMMARY_TSV}")"
OPT_TOTAL_COUNT="$(awk -F'|' 'NR>1 && $3=="optional" {c++} END{print c+0}' "${SUMMARY_TSV}")"
GATE_PASS="$(awk -F'|' 'NR>1 && $2==0 {c++} END{print c+0}' "${GATE_OUT_REL}/summary.tsv")"
GATE_TOTAL="$(awk 'END{print NR-1}' "${GATE_OUT_REL}/summary.tsv")"

cat > "${OUTDIR}/one_page.md" <<EOM
CodeM CUDA Cluster Hardware Sweep

- Date: $(date)
- Host: $(hostname)
- Conda env: ${CONDA_ENV}
- Build dir: ${BUILD_DIR}
- Seed: ${SEED}
- Stress reps: ${STRESS_REPS}

## Overall Status
- Required cluster steps passing: ${REQ_PASS_COUNT} / ${REQ_TOTAL_COUNT}
- Optional environment probes passing: ${OPT_PASS_COUNT} / ${OPT_TOTAL_COUNT}
- CUDA mega-gates passing: ${GATE_PASS} / ${GATE_TOTAL}
- CUDA mega-gate artifact: ${GATE_OUT_REL}

## Upload-Ready Files
- ${OUTDIR}/one_page.md
- ${OUTDIR}/summary.tsv
- ${OUTDIR}/cuda_gate_one_page.md
- ${OUTDIR}/cuda_gate_summary.tsv
- ${OUTDIR}/cuda_mega_gate.log

## Pointers
- ${OUT_ROOT}/latest_cuda_cluster_hardware_path.txt
- ${OUT_ROOT}/latest_cuda_mega_gate_path.txt
EOM

OUTDIR_ABS="$(cd "${OUTDIR}" && pwd)"
echo "${OUTDIR_ABS}" > "${OUT_ROOT}/latest_cuda_cluster_hardware_path.txt"
ln -sfn "$(basename "${OUTDIR}")" "${OUT_ROOT}/latest_cuda_cluster_hardware"

echo "${OUTDIR}"
