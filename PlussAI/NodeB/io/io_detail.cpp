#include "io_detail.h"

#include <iomanip>
#include <sstream>

namespace pluss::ai::nodeb::io::detail {

std::uint64_t fnv1a64(std::string_view data) noexcept {
  constexpr std::uint64_t kOffsetBasis = 14695981039346656037ull;
  constexpr std::uint64_t kPrime = 1099511628211ull;
  std::uint64_t hash = kOffsetBasis;
  for (const unsigned char byte : data) {
    hash ^= byte;
    hash *= kPrime;
  }
  return hash;
}

std::string to_hex(std::uint64_t value) {
  std::ostringstream oss;
  oss << std::hex << std::setw(16) << std::setfill('0') << value;
  return oss.str();
}

std::uint64_t parse_hex_u64(std::string_view value) {
  return static_cast<std::uint64_t>(std::stoull(std::string(value), nullptr, 16));
}

std::int64_t parse_i64(std::string_view value) {
  return static_cast<std::int64_t>(std::stoll(std::string(value)));
}

} // namespace pluss::ai::nodeb::io::detail
