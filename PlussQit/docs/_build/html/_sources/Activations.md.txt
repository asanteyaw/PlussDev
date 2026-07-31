# Activations

Torch-inspired activation functions are exposed as **Series methods**, with in-place variants:

- `relu()`, `relu_()`
- `sigmoid()`, `sigmoid_()`
- `softplus(beta=1, threshold=20)`, `softplus_(...)`
- `silu()`, `silu_()`  (a.k.a. swish)
- `gelu(approximate=true)`, `gelu_(...)`
- `leaky_relu(negative_slope=0.01)`, `leaky_relu_(...)`

Notes:
- CPU-only.
- Complex dtypes throw (Torch-consistent for these activations).
