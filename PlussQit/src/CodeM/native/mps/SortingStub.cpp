#include "CodeM/native/mps/Sorting.h"

#include "CodeM/core/Errors.h"

namespace pluss::codem::native::mps {

bool supports_sort_selection(const Series&, std::int64_t, bool) noexcept {
  return false;
}

std::pair<Series, Series> sort_with_indices(const Series&, std::int64_t, bool) {
  throw_error("MPS sorting backend is unavailable on this platform/build");
}

std::pair<Series, Series> topk_with_indices(const Series&, std::int64_t, std::int64_t, bool) {
  throw_error("MPS sorting backend is unavailable on this platform/build");
}

Series argsort(const Series&, std::int64_t, bool) {
  throw_error("MPS sorting backend is unavailable on this platform/build");
}

Series kthvalue(const Series&, std::int64_t, std::int64_t, bool) {
  throw_error("MPS sorting backend is unavailable on this platform/build");
}

} // namespace pluss::codem::native::mps
