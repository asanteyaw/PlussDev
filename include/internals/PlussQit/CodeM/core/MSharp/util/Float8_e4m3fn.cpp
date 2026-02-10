#include <MSharp/util/Float8_e4m3fn.h>
#include <type_traits>

namespace MSharp {

static_assert(
    std::is_standard_layout_v<Float8_e4m3fn>,
    "MSharp::Float8_e4m3fn must be standard layout.");

} // namespace MSharp
