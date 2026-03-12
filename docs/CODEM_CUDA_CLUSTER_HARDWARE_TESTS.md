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
- captures hardware/runtime inventory (`nvidia-smi`, `nvcc`, toolchain probes),
- runs CUDA mega-gate matrix (`25` gates),
- runs repeated stress loops for bridge + strict-policy families,
- runs repeated torch-family parity loops (`[cuda][parity][bridge]`, `[cuda][parity][bridge][stress]`, `[cuda][parity][policy]`),
- writes compact summary artifacts.

## Artifacts
Each run writes to:

- `outputs/cuda_cluster_hardware_<timestamp>/one_page.md`
- `outputs/cuda_cluster_hardware_<timestamp>/summary.tsv`
- `outputs/cuda_cluster_hardware_<timestamp>/cuda_gate_one_page.md`
- `outputs/cuda_cluster_hardware_<timestamp>/cuda_gate_summary.tsv`
- `outputs/cuda_cluster_hardware_<timestamp>/cuda_mega_gate.log`

Stable pointer:
- `outputs/latest_cuda_cluster_hardware_path.txt`

## What To Upload Here
For fast review, upload only:
- cluster `one_page.md`,
- cluster `summary.tsv`,
- gate `cuda_gate_one_page.md`,
- gate `cuda_gate_summary.tsv`.

Raw per-gate logs remain on disk in the referenced mega-gate artifact directory.
