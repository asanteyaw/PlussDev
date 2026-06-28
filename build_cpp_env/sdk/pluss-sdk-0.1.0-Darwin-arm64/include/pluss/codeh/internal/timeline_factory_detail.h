#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "pluss/codeh/codeh.h"
#include "pluss/detail/factory_containers.h"

namespace pluss::codeh::factory_detail {

inline void check_rank(pluss::detail::ShapeSpec sizes, std::string_view where) {
  CODEM_CHECK(sizes.rank() <= 3, std::string(where) + ": Timeline supports ranks up to 3");
}

inline pluss::codeh::Timeline from_textspan(pluss::detail::TextSpan flat,
                                            pluss::detail::ShapeSpec sizes) {
  check_rank(sizes, "make_timeline");
  return pluss::codeh::ops_internal::timeline_from_utf8(flat.span(), sizes.span());
}

inline pluss::codeh::Timeline from_textspan_1d(pluss::detail::TextSpan s1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(s1d.size())};
  return from_textspan(
      std::move(s1d),
      pluss::detail::ShapeSpec(std::span<const std::int64_t>(sizes.data(), sizes.size())));
}

} // namespace pluss::codeh::factory_detail
