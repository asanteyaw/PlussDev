#pragma once

#include <PlussQit/cuda/ATenCUDAGeneral.h>
#include <PlussQit/cudnn/cudnn-wrapper.h>

namespace pluss::native {

TORCH_CUDA_CPP_API cudnnHandle_t getCudnnHandle();
} // namespace pluss::native
