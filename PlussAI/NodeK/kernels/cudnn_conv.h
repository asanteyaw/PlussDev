#pragma once

#include <pluss/core.h>

#include <array>
#include <optional>
#include <string>

namespace pluss::ai::kernels::cudnn {

struct Conv2dGeometry {
  std::array<std::int64_t, 2> stride{1, 1};
  std::array<std::int64_t, 2> padding{0, 0};
  std::array<std::int64_t, 2> dilation{1, 1};
  std::int64_t groups{1};
};

bool route_enabled() noexcept;
bool route_strict() noexcept;
bool route_ready() noexcept;

bool supports_conv2d(
    const Series& input,
    const Series& weight,
    const Series& bias,
    const Conv2dGeometry& geom,
    bool require_backward,
    std::string* reason = nullptr) noexcept;

std::optional<Series> try_conv2d(
    const Series& input,
    const Series& weight,
    const Series& bias,
    const Conv2dGeometry& geom,
    bool require_backward);

} // namespace pluss::ai::kernels::cudnn
