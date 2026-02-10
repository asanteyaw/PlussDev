// Just a little test file to make sure that the CUDA library works

#include <MSharp/cuda/CUDAException.h>
#include <MSharp/cuda/impl/CUDATest.h>

#include <cuda_runtime.h>

namespace MSharp::cuda::impl {

static bool has_cuda_gpu() {
  int count = 0;
  C10_CUDA_IGNORE_ERROR(cudaGetDeviceCount(&count));

  return count != 0;
}

int c10_cuda_test() {
  int r = 0;
  if (has_cuda_gpu()) {
    C10_CUDA_CHECK(cudaGetDevice(&r));
  }
  return r;
}

} // namespace MSharp::cuda::impl
