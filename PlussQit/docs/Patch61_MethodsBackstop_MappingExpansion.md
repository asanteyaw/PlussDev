# Patch 61: Method-surface usability sprint (mapping expansion)

Patch 59 generated a large Torch-like method surface.
Patch 60 introduced variadic `invoke_any` so those methods are callable.

Patch 61 expands the mapping layer so far more generated method names route to
real CPU kernels (`ops_internal::*`) or existing `Series` methods, instead of throwing.

## Newly routed categories (selected)
- Unary: asin/acos/atan/sinh/cosh/asinh/acosh/atanh/erf/erfc/lgamma/reciprocal/trunc/isfinite/logical_not/angle
- Reductions: sum/mean/prod/all/any/amax/amin/argmax/argmin (all-dims and dim forms)
- Activations: silu/gelu/leaky_relu (+ inplace variants), softplus(beta,threshold)
- Softmax: softmax(dim) and log_softmax(dim)
- Bitwise ops routed to existing Series methods

The mapping layer still throws `NotImplemented(op(signature))` for unimplemented names,
which keeps behavior safe and explicit while we fill remaining kernels in Patch 62.
