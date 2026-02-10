#pragma once

#include <PlussQit/Series.h>
#include <PlussQit/cudnn/cudnn-wrapper.h>

namespace pluss::native {

TORCH_CUDA_CPP_API cudnnDataType_t
getCudnnDataTypeFromScalarType(const pluss::ScalarType dtype);
cudnnDataType_t getCudnnDataType(const pluss::Series& series);

int64_t cudnn_version();

} // namespace pluss::native
