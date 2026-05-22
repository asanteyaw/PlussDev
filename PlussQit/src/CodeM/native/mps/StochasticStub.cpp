#include "CodeM/native/mps/Stochastic.h"

namespace pluss::codem::native::mps {

bool supports_uniform_stochastic(ScalarType) noexcept {
  return false;
}

bool supports_normal_stochastic(ScalarType) noexcept {
  return false;
}

Series uniform_stochastic(pluss::msharp::RangeView::Int,
                          ScalarType,
                          double,
                          double,
                          std::uint64_t) {
  throw_error("mps::uniform_stochastic: MPS is unavailable on this platform");
}

Series normal_stochastic(pluss::msharp::RangeView::Int,
                         ScalarType,
                         double,
                         double,
                         std::uint64_t) {
  throw_error("mps::normal_stochastic: MPS is unavailable on this platform");
}

} // namespace pluss::codem::native::mps
