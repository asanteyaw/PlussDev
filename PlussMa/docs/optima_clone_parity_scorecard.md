# Optima Clone-Parity Scorecard

Purpose:
- provide a reproducible, executable score for SQP + InternalProtocol clone-parity checkpoints
- make reported percentages auditable from code and fixed criteria

Executable:
- `plussma_optima_clone_parity_scorecard`

Strict mode:
- `plussma_optima_clone_parity_scorecard --strict`
- exits non-zero if any checkpoint fails

## Included Checkpoints

1. `GARCH N=40 SQP`
2. `GARCH N=40 InternalProtocol`
3. `GARCH N=200 SQP`
4. `GARCH N=200 InternalProtocol`
5. `GARCH N=800 SQP`
6. `GARCH N=800 InternalProtocol`
7. `Structured n=64 SQP`
8. `Structured n=64 InternalProtocol`

## Pass Criteria

- common feasibility requirement: `max_constraint_violation <= 1e-8`
- GARCH SQP tiers: `exit == Converged`
- GARCH InternalProtocol N=200/N=800:
  - `exit == Converged`
  - `first_order_optimality <= 5e-3`
- GARCH InternalProtocol N=40:
  - pass if either:
    - `exit == Converged`, or
    - `exit == MaxIterations` with:
      - objective gap to SQP `<= 1e-2`
      - `first_order_optimality <= 5e-3`
- Structured `n=64` SQP/InternalProtocol:
  - `exit == Converged`
  - `max |x - target| <= 1e-2`

Score definition:
- `score = passed_checkpoints / total_checkpoints * 100`

## Notes

- this scorecard is intentionally deterministic (fixed seeds and fixed options)
- it complements, not replaces, the full benchmark tables:
  - `optima_clone_parity_benchmarks.md`
  - `optima_structured_clone_parity_benchmarks.md`
  - `optima_pathology_clone_parity_benchmarks.md`
