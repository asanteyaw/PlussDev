# CodeM (ATen-equivalent)

This folder is the **ATen** analog for PlussQit.

Nomenclature mapping used here:
- Tensor -> Series
- torch/TORCH -> pluss/PLUSS
- c10 -> MSharp
- caffe2 -> sm

CPU-only for now. Other backends can be added by extending dispatch keys and native kernels.
