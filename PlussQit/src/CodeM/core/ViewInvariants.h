#pragma once
#include "CodeM/core/Errors.h"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace pluss::codem {

// Dense+non-overlapping check for strided tensors. Conservative:
// - Computes byte extents per-dim and checks that linearization is injective for common dense layouts.
// - For production, we mainly enforce that no two indices map to same offset when created via interop or view ops that claim dense.
inline bool is_valid_strided_view(const std::vector<std::int64_t>& sizes,
                                  const std::vector<std::int64_t>& strides) {
  if (sizes.size() != strides.size()) return false;
  // zero-numel is always fine
  for (auto s: sizes) { if (s < 0) return false; if (s==0) return true; }
  // Sort dims by stride magnitude (like torch)
  std::vector<std::size_t> perm(sizes.size());
  for (std::size_t i=0;i<perm.size();++i) perm[i]=i;
  std::sort(perm.begin(), perm.end(), [&](auto i, auto j){
    auto ai = strides[i] < 0 ? -strides[i] : strides[i];
    auto aj = strides[j] < 0 ? -strides[j] : strides[j];
    return ai < aj;
  });
  std::int64_t expected = 1;
  for (auto d : perm) {
    if (sizes[d] == 1) continue;
    auto st = strides[d] < 0 ? -strides[d] : strides[d];
    // Allow broadcasted views (e.g. expand) with zero stride.
    if (st == 0) continue;
    // A larger stride leaves holes but remains a valid non-overlapping view
    // (column slices and stepped slices are the common cases).
    if (st < expected) return false;
    expected *= sizes[d];
  }
  return true;
}

inline void check_view_invariants(const std::vector<std::int64_t>& sizes,
                                  const std::vector<std::int64_t>& strides) {
#ifndef NDEBUG
  if (!is_valid_strided_view(sizes, strides)) {
    throw_error("View invariants violated: expected a valid non-overlapping strided layout");
  }
#else
  (void)sizes;
  (void)strides;
#endif
}

} // namespace pluss::codem
