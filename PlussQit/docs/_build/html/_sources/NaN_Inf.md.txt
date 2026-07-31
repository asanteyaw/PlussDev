# NaN / Inf behavior

PlussQit follows IEEE-754 behavior for floating types:

- Float32/Float64: native IEEE-754
- Float16/BFloat16: computed through float32 conversion where needed; NaN/Inf preserved
- Float8:
  - E5M2 supports Inf and NaN
  - E4M3FN has no Inf; large values saturate; NaNs supported

## Operations

- Floating division by 0 yields Inf/NaN per IEEE rules.
- Integral division by 0 throws (defined error).
- Adding a non-numeric C++ type (e.g., string literal) is a compile-time error, not a runtime NaN.
