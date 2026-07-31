# Norms

Torch-inspired norms are available as Series methods:

- `x.norm(p=2)`
- `x.norm(p, dim, keepdim=false)`

Supported:
- p = 1, 2, +inf
- general p for real floating types (implemented via abs/pow/sum)

Notes:
- Complex norms are not implemented yet in this patch.
