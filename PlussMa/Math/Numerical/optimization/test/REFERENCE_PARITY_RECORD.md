# External Reference Parity Record

Date: February 26, 2026

This file snapshots the last external-reference parity run before removing vendor folders.

## Reference Revisions

- `nomad-master` HEAD: `31ca9eb638a78f393a61104c22eb50a38b26598a`
- `optim-master` HEAD: `31ca9eb638a78f393a61104c22eb50a38b26598a`

## External Parity Run

Command:

```bash
./build/PlussMa/Math/Numerical/optimization/test/external_reference_parity_test
```

Observed output:

```text
[SKIP] optim-master external parity is unavailable (missing include/BaseMatrixOps submodule content)
nomad_ref_obj=-1995.84 pluss_obj=-2000 abs_diff=4.16
external_reference_parity_test: all checks passed
```

## In-Tree NOMAD Parity Run

Command:

```bash
./build/PlussMa/Math/Numerical/optimization/test/mads_nomad_parity_test
```

Observed output:

```text
orthomads_quartic f=0 evals=100
mads_nomad_parity_test: all checks passed
ltmads_quartic f=0 evals=100
```
