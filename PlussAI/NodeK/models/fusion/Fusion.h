#pragma once

#include <pluss/nn_detail.h>

namespace pluss::ai {

inline Series ff(const Series& input, const Series& weight, const Series& bias = Series()) {
  return ::pluss::detail::nn::affine_(input, weight, bias, "ff");
}

inline Series dropout(const Series& x, double p, bool training = true, std::uint64_t seed = 0) {
  return ::pluss::detail::nn::dropout_(x, p, training, seed);
}

} // namespace pluss::ai
