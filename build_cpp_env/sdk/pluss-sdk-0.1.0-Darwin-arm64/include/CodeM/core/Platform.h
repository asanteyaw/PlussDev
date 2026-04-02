#pragma once

#include <cstdint>
#include <ostream>

namespace pluss::codem {

enum class Platform : std::uint8_t {
  CPU = 0,
  CUDA = 1,
  MPS = 2
};

inline std::ostream& operator<<(std::ostream& os, Platform p) {
  switch (p) {
    case Platform::CPU:  os << "cpu"; break;
    case Platform::CUDA: os << "cuda"; break;
    case Platform::MPS:  os << "mps"; break;
    default:             os << "unknown"; break;
  }
  return os;
}

} // namespace pluss::codem
