#include <MSharp/cuda/CUDAException.h>

#include <MSharp/cuda/CUDADeviceAssertionHost.h>
#include <MSharp/util/Exception.h>
#include <cuda_runtime.h>

#include <string>

namespace MSharp::cuda {

void c10_cuda_check_implementation(
    const int32_t err,
    const char* filename,
    const char* function_name,
    const int line_number,
    const bool include_device_assertions) {
  const auto cuda_error = static_cast<cudaError_t>(err);
  const auto cuda_kernel_failure = include_device_assertions
      ? MSharp::cuda::CUDAKernelLaunchRegistry::get_singleton_ref().has_failed()
      : false;

  if (C10_LIKELY(cuda_error == cudaSuccess && !cuda_kernel_failure)) {
    return;
  }

  [[maybe_unused]] auto error_unused = cudaGetLastError();
  (void)error_unused;

  std::string check_message;
#ifndef STRIP_ERROR_MESSAGES
  check_message.append("CUDA error: ");
  check_message.append(cudaGetErrorString(cuda_error));
  check_message.append(MSharp::cuda::get_cuda_check_suffix());
  check_message.append("\n");
  if (include_device_assertions) {
    check_message.append(c10_retrieve_device_side_assertion_info());
  } else {
    check_message.append(
        "Device-side assertions were explicitly omitted for this error check; the error probably arose while initializing the DSA handlers.");
  }
#endif

  TORCH_CHECK(false, check_message);
}

} // namespace MSharp::cuda
