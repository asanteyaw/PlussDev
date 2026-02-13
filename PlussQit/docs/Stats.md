# Statistics: var / std

Torch-inspired statistical ops:

- `x.var(unbiased=false)`
- `x.var(dim, unbiased=false, keepdim=false)`
- `x.std(unbiased=false)`
- `x.std(dim, unbiased=false, keepdim=false)`

Current behavior:
- floating dtypes only (Float16/BFloat16/Float32/Float64)
- implemented via mean + (x-mean)^2 reduction

## Numerical stability

Global variance for Float32/Float64 uses a Welford one-pass algorithm for improved numerical stability.
