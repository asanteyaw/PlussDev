#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/native/IndexingUtils.h>

namespace pluss::native {

bool canUse32BitIndexMath(const TensorBase& t, int64_t max_elem) {
  auto elements = t.sym_numel();
  if (elements >= max_elem) {
    return false;
  }
  if (elements == 0) {
    return max_elem > 0;
  }

  MSharp::SymInt offset = 0;
  auto linearId = elements - 1;

  // NOTE: Assumes all strides are positive, which is true for now
  // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
  for (int i = t.dim() - 1; i >= 0; --i) {
    auto curDimIndex = linearId % t.sym_size(i);
    auto curDimOffset = curDimIndex * t.sym_stride(i);
    offset += curDimOffset;
    linearId /= t.sym_size(i);
  }

  if (offset >= max_elem) {
    return false;
  }

  return true;
}

} // namespace pluss::native
