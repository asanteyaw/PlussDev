# Patch 55: Indexing contiguous-advanced parity + logical/mask ops

## Indexing
Implements Torch/Numpy rule:
- If advanced index tensors are contiguous in the index list, the broadcasted advanced shape stays **in place**
  rather than being moved to the front.

Implementation lives in `native/cpu/AdvancedIndexing.cpp`:
- `index_torch_like` and `index_put_torch_like` detect contiguous advanced blocks and apply a
  permute-to-front / permute-back strategy so the output layout matches Torch’s "contiguous advanced dims" behavior.

## Logical ops
Adds `logical_and`, `logical_or`, `logical_xor` (bool tensors, broadcasting supported).

## Mask ops
Adds:
- `masked_select`
- `masked_fill` and `masked_fill_` (in-place requires mask broadcast shape to match the tensor for safety)

All are exposed as **Series methods** via OpsInternal + codegen.
