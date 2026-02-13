# Type promotion

PlussQit implements a Torch-inspired (CPU-only) type promotion policy in:

- `src/CodeM/core/TypePromotion.{h,cpp}`

Current rules (simplified but explicit):
- Complex dominates.
- Floating dominates integral/bool.
- Integral/bool combinations widen safely to Int64.
- Float8 storage promotes to Float16 at op boundaries (compute type decisions may evolve).

This file is the single place to evolve toward full Torch parity.

## Notes (Patch 51)

- Float16/BFloat16 mixed with Float32 promotes to Float32.
- Any mix with Float64 promotes to Float64.
- UInt8 with signed integers promotes to Int64 (safe policy).
