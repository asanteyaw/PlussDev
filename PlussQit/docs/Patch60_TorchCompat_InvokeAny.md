# Patch 60: Torch-compat invoke_any (variadic) to make generated method surface usable

Patch 59 generated thousands of `Series` method names from Torch YAML. Patch 60 makes those methods
*actually callable* without compile-time arity/type restrictions by introducing a small type-erased
argument layer:

- `torch_compat::Arg` and `torch_compat::Args`
- `invoke_any(op, self, args)` and `invoke_any_(...)`

## Supported argument kinds
- `Series`
- scalar numbers (`double`, `int64`)
- `bool`
- `string` / `string_view`
- `initializer_list<int64>` / `vector<int64>` (for sizes / dims)

## What is implemented vs. what throws
`invoke_any` maps a growing set of common Torch method names to real CPU kernels:
- unary: abs/neg/exp/log/sqrt/floor/ceil/round/sin/cos/tan/tanh/relu/sigmoid/softplus/conj/isnan/isinf
- binary: add/sub/mul/div/pow/comparisons/logical_*
- reshape/view/permute/transpose/contiguous/clone
- reductions: sum/mean/amax/amin/argmax/argmin with `(dim, keepdim)` routing

All other generated method names throw a precise:
`Torch-compat method not implemented (CPU-only build): op(argtypes...)`

This patch is a deliberate stepping stone: Patch 61/62 will implement large reference-kernel backstops
so most methods stop throwing.
