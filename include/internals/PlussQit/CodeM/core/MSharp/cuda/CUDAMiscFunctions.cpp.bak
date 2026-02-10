#include <MSharp/cuda/CUDAMiscFunctions.h>
#include <MSharp/util/env.h>

namespace MSharp::cuda {

// NOLINTNEXTLINE(bugprone-exception-escape,-warnings-as-errors)
const char* get_cuda_check_suffix() noexcept {
  static auto device_blocking_flag =
      MSharp::utils::check_env("CUDA_LAUNCH_BLOCKING");
  static bool blocking_enabled =
      (device_blocking_flag.has_value() && device_blocking_flag.value());
  if (blocking_enabled) {
    return "";
  } else {
    return "\nCUDA kernel errors might be asynchronously reported at some"
           " other API call, so the stacktrace below might be incorrect."
           "\nFor debugging consider passing CUDA_LAUNCH_BLOCKING=1";
  }
}
std::mutex* getFreeMutex() {
  static std::mutex cuda_free_mutex;
  return &cuda_free_mutex;
}

} // namespace MSharp::cuda
