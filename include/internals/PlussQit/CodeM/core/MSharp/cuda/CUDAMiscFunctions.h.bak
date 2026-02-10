#pragma once
// this file is to avoid circular dependency between CUDAFunctions.h and
// CUDAExceptions.h

#include <MSharp/cuda/CUDAMacros.h>

#include <mutex>

namespace MSharp::cuda {
C10_CUDA_API const char* get_cuda_check_suffix() noexcept;
C10_CUDA_API std::mutex* getFreeMutex();
} // namespace MSharp::cuda
