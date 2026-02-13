# Reshaping and views

PlussQit follows Torch-inspired semantics, with a stricter safety policy (CPU-only).

## Methods

- `view(new_sizes)` : requires viewable layout (currently contiguous requirement)
- `reshape(new_sizes)` : view if possible, else returns `contiguous().view(...)`
- `transpose(d0, d1)` : returns a view with swapped sizes/strides
- `t()` : 2D transpose shortcut
- `permute(dims...)` : returns a view reordering dimensions
- `unsqueeze(dim)` : inserts a size-1 dimension (view)
- `squeeze()` / `squeeze(dim)` : removes size-1 dimensions (view)
- `flatten(start_dim=0, end_dim=-1)` : reshapes by merging a range of dims
