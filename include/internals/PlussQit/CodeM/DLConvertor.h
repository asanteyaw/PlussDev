#pragma once

#include <PlussQit/PlussQit.h>
#include <PlussQit/Series.h>
#include <PlussQit/dlpack.h>

// this convertor will:
// 1) take a Series object and wrap it in the DLPack series
// 2) take a dlpack series and convert it to the PlussQit Series

namespace pluss {

TORCH_API ScalarType toScalarType(const DLDataType& dtype);
TORCH_API DLManagedTensor* toDLPack(const Series& src);
TORCH_API Series fromDLPack(DLManagedTensor* src);
TORCH_API Series
fromDLPack(DLManagedTensor* src, std::function<void(void*)> deleter);
TORCH_API DLDataType getDLDataType(const Series& t);
TORCH_API DLDevice getDLContext(const Series& series, const int64_t& device_id);

} // namespace pluss
