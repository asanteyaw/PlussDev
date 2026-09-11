#pragma once

namespace pluss {

struct null_t final {
  constexpr null_t() noexcept = default;
};

namespace series {

inline constexpr null_t null{};

} // namespace series

} // namespace pluss
