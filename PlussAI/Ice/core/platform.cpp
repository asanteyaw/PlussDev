#include "pluss/ai/ice/platform.h"

namespace pluss::ai::ice {

Platform resolve_platform(TrainPlatform requested) {
  if (requested == TrainPlatform::CPU) {
    return pluss::pCPU;
  }
  if (pluss::cuda::is_ready()) return pluss::pCUDA;
  if (pluss::mps::is_ready()) return pluss::pMPS;
  if (requested == TrainPlatform::GPU) {
    pluss::validate::warn(
        "pluss::ai::ice: TrainPlatform::GPU was requested but no GPU (CUDA or MPS) is available "
        "on this machine; falling back to CPU for training.");
  }
  return pluss::pCPU;
}

} // namespace pluss::ai::ice
