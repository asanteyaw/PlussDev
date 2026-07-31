# scatter_reduce policies

PlussQit scatter_reduce matches Torch-style knobs in the CPU-only setting.

## include_self

- `include_self=true` (default): output is initialized from `self`
- `include_self=false`: output is initialized from `initial_scalar` if provided, otherwise from the identity

## initial_scalar

Optional scalar initial value applied to every output element when `include_self=false`.

## dtype support

- Sum/amax/amin: supported for numeric real and integer dtypes
- Complex: only Sum (amax/amin are not defined)
- Float16/BFloat16: compute in float and cast back (preserves NaN/Inf)
