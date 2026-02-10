#include <MSharp/util/Half.h>
#include <type_traits>

namespace MSharp {

static_assert(
    std::is_standard_layout_v<Half>,
    "MSharp::Half must be standard layout.");

} // namespace MSharp
