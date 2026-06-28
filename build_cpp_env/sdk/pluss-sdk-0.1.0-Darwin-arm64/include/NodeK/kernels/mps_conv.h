#pragma once

#include <pluss/core.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace pluss::ai::kernels::mps {

bool conv3d_route_enabled() noexcept;
bool conv3d_route_strict() noexcept;
bool conv3d_route_ready() noexcept;
std::uint64_t conv3d_native_call_count() noexcept;

bool supports_conv3d(const Series& input,
                     const Series& weight,
                     const Series& bias,
                     std::array<std::int64_t, 3> stride,
                     std::array<std::int64_t, 3> padding,
                     std::array<std::int64_t, 3> dilation,
                     std::int64_t groups,
                     bool require_backward,
                     std::string* reason = nullptr) noexcept;

std::optional<Series> try_conv3d(const Series& input,
                                 const Series& weight,
                                 const Series& bias,
                                 std::array<std::int64_t, 3> stride,
                                 std::array<std::int64_t, 3> padding,
                                 std::array<std::int64_t, 3> dilation,
                                 std::int64_t groups,
                                 bool require_backward);

} // namespace pluss::ai::kernels::mps
