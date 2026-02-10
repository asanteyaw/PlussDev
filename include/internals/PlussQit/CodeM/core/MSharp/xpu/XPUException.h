#pragma once

#include <MSharp/util/Exception.h>
#include <sycl/sycl.hpp>

namespace MSharp::xpu {

static inline sycl::async_handler asyncHandler = [](sycl::exception_list el) {
  if (el.size() == 0) {
    return;
  }
  for (const auto& e : el) {
    try {
      std::rethrow_exception(e);
    } catch (sycl::exception& e) {
      TORCH_WARN("SYCL Exception: ", e.what());
    }
  }
  throw;
};

} // namespace MSharp::xpu
