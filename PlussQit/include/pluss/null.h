#pragma once

namespace pluss {

struct null_t final {
  constexpr null_t() noexcept = default;
};

inline constexpr null_t null{};

} // namespace pluss
