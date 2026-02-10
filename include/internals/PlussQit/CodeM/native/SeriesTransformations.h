#include <PlussQit/core/Series.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#else
#include <PlussQit/ops/roll.h>
#endif

#include <MSharp/util/Exception.h>

namespace pluss::native {

static inline Series roll_common(
    const Series& self,
    IntArrayRef shifts,
    IntArrayRef dims) {
  TORCH_CHECK(!shifts.empty(), "`shifts` required");
  if (dims.empty() && shifts.size() == 1) {
    auto flattened = self.contiguous().view(self.numel());
    return roll(flattened, shifts[0], 0).view(self.sizes());
  }
  TORCH_CHECK(
      shifts.size() == dims.size(),
      "shifts and dimensions must align. shifts: ",
      shifts.size(),
      ", dims:",
      dims.size());
  AT_ASSERT(dims.size() > 1);
  auto tail_shifts = shifts.slice(1);
  auto tail_dims = dims.slice(1);
  auto first_dim_rolled = roll(self, shifts[0], dims[0]);
  return pluss::roll(first_dim_rolled, tail_shifts, tail_dims);
}

} // namespace pluss::native
