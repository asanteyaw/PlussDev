# Float8 policy

PlussQit supports Float8 as *storage* dtypes on CPU:

- `ScalarType::Float8E4M3FN`
- `ScalarType::Float8E5M2`

## Compute policy (CPU)

On CPU, elementwise and reduction kernels **upcast Float8 inputs to Float32**, compute in Float32,
then (if output dtype is Float8) convert back to Float8.

This mirrors common production practice: Float8 is useful for memory bandwidth, but CPU compute
in Float8 is rarely efficient or available.

## NaN/Inf behavior

- E5M2 supports Inf and NaN (IEEE-like special encodings).
- E4M3FN has no Inf; large values saturate; NaNs are supported.
