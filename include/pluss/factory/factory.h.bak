#pragma once

#include "value.h"
#include "value_options.h"

namespace pluss {

// === Scalar & init-list construction ===
Value value(const std::initializer_list<std::initializer_list<double>>& list, const Options& opt = {});
Value value(const std::initializer_list<int> list, const Options& opt = {});
Value value(const std::initializer_list<double> list, const Options& opt = {});
Value value(const double x, const Options& opt = {});
Value value(const int x, const Options& opt = {});

// === Std vector-based construction ===
Value from_vec(const std::vector<float>& list, const Options& opt = {});
Value from_vec(const std::vector<int32_t>& list, const Options& opt = {});
Value from_vec(const std::vector<double>& list, const Options& opt = {});
Value from_vec(const std::vector<std::vector<double>>& list, const Options& opt = {});
Value from_vec(const std::vector<std::vector<std::vector<double>>>& list, const Options& opt = {});

// === Random / Torch bridge ===
Value rand(const std::vector<int64_t>& shape, const Options& opt = {});
Value randn(const std::vector<int64_t>& shape, const Options& opt = {});
Value from_torch(const torch::Series& t);

}  // namespace pluss