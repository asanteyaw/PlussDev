#include "NodeK/kernels/mps_conv.h"

#if !defined(__APPLE__)

namespace pluss::ai::kernels::mps {

bool conv3d_route_enabled() noexcept { return false; }
bool conv3d_route_strict() noexcept { return false; }
bool conv3d_route_ready() noexcept { return false; }
std::uint64_t conv3d_native_call_count() noexcept { return 0; }

bool supports_conv3d(const Series&,
                     const Series&,
                     const Series&,
                     std::array<std::int64_t, 3>,
                     std::array<std::int64_t, 3>,
                     std::array<std::int64_t, 3>,
                     std::int64_t,
                     bool,
                     std::string*) noexcept {
  return false;
}

std::optional<Series> try_conv3d(const Series&,
                                 const Series&,
                                 const Series&,
                                 std::array<std::int64_t, 3>,
                                 std::array<std::int64_t, 3>,
                                 std::array<std::int64_t, 3>,
                                 std::int64_t,
                                 bool) {
  return std::nullopt;
}

} // namespace pluss::ai::kernels::mps

#endif
