# Patch 62: Reference kernels + ops_internal expansion for more Torch methods

This patch adds additional CPU reference kernels (strided, broadcast-aware via SeriesIterator)
and wires them through `ops_internal` so that many more generated Torch method names stop throwing.

## New kernels
- Unary: rsqrt, log2, log10, exp2, cbrt, square
- Binary: atan2, fmod, remainder (Torch sign semantics for remainder)

## New ops_internal APIs
- maximum/minimum (wired to existing MinMaxClamp kernels)
- atan2/fmod/remainder
- rsqrt/log2/log10/exp2/cbrt/square
- clamp_min/clamp_max (scalar, plus inplace variants)
- nan_to_num (+ inplace)

## Impact
The Torch-method surface generated in Patch 59 now routes a larger fraction of methods to
real kernels instead of throwing `NotImplemented`.
