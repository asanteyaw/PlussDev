# Patch 65: Factory dtype policy + namespace hygiene + where/nonzero + gather/scatter routing

## Factory policy (professional-grade)
`pluss::value` now **deduces dtype from the leaf type** of nested initializer_lists unless the user specifies a dtype:
- all-integral lists -> Int64 (Torch-like)
- bool lists -> Bool
- float lists -> Float32
- double lists -> Float64
- explicit dtype always overrides (casts/promotes)

This avoids surprising coercions like `{true,false}` becoming Float32 by default.

Implementation lives in `include/pluss/detail/factory_deduction.h` so all future factories can share a single, auditable policy surface.

## Namespace hygiene
All public factories (`value`, `tensor`, `make_series`, `from_blob`, etc.) are now declared **inside** `namespace pluss`.

## masked_select parity improvement
`ops_internal::masked_select` now broadcasts the mask to the input shape before selection (Torch-like).

## where variants + nonzero
- `Series condition.where(x, y)` is supported (method form).
- `Series::nonzero()` returns int64 indices (`nnz x ndim`) with a scalar policy compatible with Torch expectations.

## gather/scatter routing
`torch_compat::invoke_any` routes more signatures toward:
- `gather(dim, index)`
- `scatter_ / scatter_add_` (in-place, 3-arg form)
