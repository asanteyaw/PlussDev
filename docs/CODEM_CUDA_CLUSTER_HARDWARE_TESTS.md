# CodeM CUDA Cluster Hardware Tests

## Goal
Run the same rigorous CUDA parity gates on a CUDA host and produce compact artifacts that can be shared here without dumping thousands of raw lines.

## Runner
Use:

```bash
tools/run_codem_cuda_cluster_hardware.sh --env cpp_env --build-dir build_cpp_env --stress-reps 5
```

Key behaviors:
- configures/builds CodeM tests with `PLUSSQIT_ENABLE_CUDA=ON`,
- builds/runs the CUDA-only gate binary `plussqit_codem_cuda_gate_tests` (no MPS test sources),
- captures hardware/runtime inventory (`nvidia-smi`, `nvcc`, toolchain probes),
- runs CUDA mega-gate matrix (`73` gates),
- runs repeated stress loops for bridge + strict-policy families,
- runs repeated no-bridge loops (`[cuda][no_bridge]`) with `PLUSS_CUDA_ALLOW_CPU_BRIDGE=0` and `PLUSS_CUDA_FORBID_CPU_MATERIALIZATION=1`,
- runs repeated device-storage loops (`[cuda][device_storage]`) with `PLUSS_CUDA_DEVICE_STORAGE=1`,
- runs repeated reduction bridge + strict-policy loops (`[cuda][bridge][reduction]`, `[cuda][bridge][policy][reduction]`),
- runs repeated elementwise bridge + strict-policy loops (`[cuda][bridge][elementwise]`, `[cuda][bridge][policy][elementwise]`),
- runs repeated scan bridge + strict-policy loops (`[cuda][bridge][scan]`, `[cuda][bridge][policy][scan]`),
- runs repeated activation/logical bridge + strict-policy loops (`[cuda][bridge][activation]`, `[cuda][bridge][policy][activation]`),
- runs repeated masking bridge + strict-policy loops (`[cuda][bridge][masking]`, `[cuda][bridge][policy][masking]`),
- runs repeated bitwise bridge + strict-policy loops (`[cuda][bridge][bitwise]`, `[cuda][bridge][policy][bitwise]`),
- runs repeated compose bridge + strict-policy loops (`[cuda][bridge][compose]`, `[cuda][bridge][policy][compose]`),
- runs repeated batched-linalg bridge + strict-policy loops (`[cuda][bridge][linalg][batched]`, `[cuda][bridge][policy][linalg][batched]`),
- runs repeated expanded-linalg bridge + strict-policy loops (`[bridge][cuda][expanded][linalg]`, `[bridge][cuda][expanded][linalg][policy]`),
- runs repeated rank-aware matmul bridge + strict-policy loops (`[cuda][bridge][matmul][rank]`, `[cuda][bridge][policy][matmul][rank]`),
- runs repeated torch-family parity loops (`[cuda][parity][bridge]`, `[cuda][parity][bridge][stress]`, `[cuda][parity][policy]`),
- runs repeated strict-native parity loops (`[cuda][parity][native][strict]`, `[cuda][parity][native][soak]`),
- runs repeated strict-native lane-expansion loop (`[cuda][parity][native][expanded]`),
- runs repeated provider stability loop (`[cuda][providers][inventory][stress]`),
- writes compact summary artifacts.

## Artifacts
Each run writes to:

- `outputs/cuda_cluster_hardware_<timestamp>/one_page.md`
- `outputs/cuda_cluster_hardware_<timestamp>/summary.tsv`
- `outputs/cuda_cluster_hardware_<timestamp>/cuda_gate_one_page.md`
- `outputs/cuda_cluster_hardware_<timestamp>/cuda_gate_summary.tsv`
- `outputs/cuda_cluster_hardware_<timestamp>/cuda_mega_gate.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_no_bridge_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_device_storage_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_reduction_bridge_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_reduction_policy_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_scan_bridge_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_scan_policy_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_activation_bridge_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_activation_policy_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_masking_bridge_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_masking_policy_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_bitwise_bridge_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_bitwise_policy_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_compose_bridge_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_compose_policy_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_linalg_batched_bridge_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_linalg_batched_policy_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_linalg_expanded_bridge_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_linalg_expanded_policy_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_matmul_rank_bridge_rep*.log`
- `outputs/cuda_cluster_hardware_<timestamp>/stress_matmul_rank_policy_rep*.log`

Stable pointer:
- `outputs/latest_cuda_cluster_hardware_path.txt`

## What To Upload Here
For fast review, upload only:
- cluster `one_page.md`,
- cluster `summary.tsv`,
- gate `cuda_gate_one_page.md`,
- gate `cuda_gate_summary.tsv`.

Raw per-gate logs remain on disk in the referenced mega-gate artifact directory.
