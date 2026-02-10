#include <MSharp/util/BFloat16.h>
#include <type_traits>

namespace MSharp {

static_assert(
    std::is_standard_layout_v<BFloat16>,
    "MSharp::BFloat16 must be standard layout.");

} // namespace MSharp
