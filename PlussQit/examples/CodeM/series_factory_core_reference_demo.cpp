#include <pluss/pluss.h>

#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

}  // namespace

int main() {
  heading("make_scalar");
  std::cout << pluss::series::make(42.5) << "\n\n";

  heading("make_scalar_int");
  std::cout << pluss::series::make(7) << "\n\n";

  heading("make_vector");
  std::cout << pluss::series::make({10.0, 12.5, 9.0}) << "\n\n";

  heading("make_bool_vector");
  std::cout << pluss::series::make({true, false, true, true}) << "\n\n";

  heading("make_matrix");
  std::cout << pluss::series::make({
      {1.0, 2.0},
      {3.0, 4.0},
  }) << "\n\n";

  heading("make_int_matrix_inferred");
  std::cout << pluss::series::make({
      {1, 2},
      {3, 4},
  }) << "\n\n";

  heading("make_flat_with_shape");
  std::cout << pluss::series::make(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2}) << "\n\n";

  heading("make_flat_rank3");
  std::cout << pluss::series::make(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {1, 2, 3}) << "\n\n";

  heading("make_dtype_override");
  std::cout << pluss::series::make(std::vector<int>{1, 2, 3, 4}, {2, 2}, pluss::ScalarType::Float64) << "\n\n";

  heading("make_config_override");
  std::cout << pluss::series::make(
      std::vector<int>{1, 2, 3, 4},
      {2, 2},
      pluss::Config{}.with_dtype(pluss::ScalarType::Int16)) << "\n\n";

  heading("arange_end");
  std::cout << pluss::series::arange(5) << "\n\n";

  heading("arange_start_end");
  std::cout << pluss::series::arange(2, 7) << "\n\n";

  heading("arange_step");
  std::cout << pluss::series::arange(10, 0, -3) << "\n\n";

  heading("arange_even_step");
  std::cout << pluss::series::arange(0, 10, 2) << "\n\n";

  heading("arange_empty");
  std::cout << pluss::series::arange(5, 5) << "\n\n";

  heading("arange_config_float32");
  std::cout << pluss::series::arange(
      2,
      7,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32)) << "\n\n";

  heading("linspace_inclusive_endpoints");
  std::cout << pluss::series::linspace(-1.0, 1.0, 5, pluss::ScalarType::Float64) << "\n\n";

  heading("linspace_singleton");
  std::cout << pluss::series::linspace(3.5, 9.0, 1, pluss::ScalarType::Float64) << "\n\n";

  heading("linspace_descending");
  std::cout << pluss::series::linspace(3.0, -3.0, 4, pluss::ScalarType::Float64) << "\n\n";

  heading("linspace_flat_interval");
  std::cout << pluss::series::linspace(2.5, 2.5, 4, pluss::ScalarType::Float64) << "\n\n";

  heading("linspace_two_points");
  std::cout << pluss::series::linspace(10.0, 20.0, 2, pluss::ScalarType::Float64) << "\n\n";

  heading("eye_square");
  std::cout << pluss::series::eye(3, pluss::ScalarType::Float64) << "\n\n";

  heading("eye_rectangular");
  std::cout << pluss::series::eye(2, 4, pluss::ScalarType::Int32) << "\n\n";

  heading("eye_bool_square");
  std::cout << pluss::series::eye(3, pluss::ScalarType::Bool) << "\n\n";

  heading("eye_singleton");
  std::cout << pluss::series::eye(1, pluss::ScalarType::Float64) << "\n\n";

  heading("eye_config_int16");
  std::cout << pluss::series::eye(3, pluss::Config{}.with_dtype(pluss::ScalarType::Int16)) << "\n\n";

  heading("linspace_invalid_steps");
  try {
    std::cout << pluss::series::linspace(0.0, 1.0, 0) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("rand_seeded");
  std::cout << pluss::series::rand({4}, pluss::ScalarType::Float64, 42) << "\n\n";

  heading("rand_matrix_seeded");
  std::cout << pluss::series::rand({2, 3}, pluss::ScalarType::Float32, 42) << "\n\n";

  heading("rand_scalar_seeded");
  std::cout << pluss::series::rand({}, pluss::ScalarType::Float64, 42) << "\n\n";

  heading("rand_same_seed_replay");
  auto rand_a = pluss::series::rand({4}, pluss::ScalarType::Float32, 7);
  auto rand_b = pluss::series::rand({4}, pluss::ScalarType::Float32, 7);
  std::cout << rand_a << "\n" << rand_b << "\n\n";

  heading("rand_global_seed_replay");
  pluss::rng_seed(424242u);
  auto rand_g0 = pluss::series::rand({4}, pluss::ScalarType::Float32);
  pluss::rng_seed(424242u);
  auto rand_g1 = pluss::series::rand({4}, pluss::ScalarType::Float32);
  std::cout << rand_g0 << "\n" << rand_g1 << "\n\n";

  heading("rand_config_float32");
  std::cout << pluss::series::rand(
      {4},
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
      42) << "\n\n";

  heading("randn_seeded");
  std::cout << pluss::series::randn({4}, pluss::ScalarType::Float64, 42) << "\n\n";

  heading("randn_matrix_seeded");
  std::cout << pluss::series::randn({2, 2}, pluss::ScalarType::Float32, 42) << "\n\n";

  heading("randn_scalar_seeded");
  std::cout << pluss::series::randn({}, pluss::ScalarType::Float64, 42) << "\n\n";

  heading("randn_same_seed_replay");
  auto randn_a = pluss::series::randn({4}, pluss::ScalarType::Float32, 9);
  auto randn_b = pluss::series::randn({4}, pluss::ScalarType::Float32, 9);
  std::cout << randn_a << "\n" << randn_b << "\n\n";

  heading("randn_config_float32");
  std::cout << pluss::series::randn(
      {4},
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
      42) << "\n\n";

  heading("randn_global_seed_replay");
  pluss::rng_seed(515151u);
  auto randn_g0 = pluss::series::randn({4}, pluss::ScalarType::Float32);
  pluss::rng_seed(515151u);
  auto randn_g1 = pluss::series::randn({4}, pluss::ScalarType::Float32);
  std::cout << randn_g0 << "\n" << randn_g1 << "\n\n";

  heading("randint_half_open");
  std::cout << pluss::series::randint(3, 8, {6}, pluss::ScalarType::Int64, 42) << "\n\n";

  heading("randint_small_range");
  std::cout << pluss::series::randint(0, 2, {8}, pluss::ScalarType::Int32, 42) << "\n\n";

  heading("randint_matrix_seeded");
  std::cout << pluss::series::randint(10, 20, {2, 3}, pluss::ScalarType::Int64, 42) << "\n\n";

  heading("randint_same_seed_replay");
  auto randint_a = pluss::series::randint(3, 8, {4}, pluss::ScalarType::Int64, 11);
  auto randint_b = pluss::series::randint(3, 8, {4}, pluss::ScalarType::Int64, 11);
  std::cout << randint_a << "\n" << randint_b << "\n\n";

  heading("randint_config_float32_normalizes");
  std::cout << pluss::series::randint(
      3,
      8,
      {4},
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
      42) << "\n\n";

  heading("randint_invalid_bounds");
  try {
    std::cout << pluss::series::randint(5, 5, {3}, pluss::ScalarType::Int64, 42) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("rand_btw_seeded");
  std::cout << pluss::series::rand_btw(-2.0, 2.0, {4}, 42) << "\n\n";

  heading("rand_btw_matrix_seeded");
  std::cout << pluss::series::rand_btw(-1.0, 1.0, {2, 2}, 42) << "\n\n";

  heading("rand_btw_degenerate_interval");
  std::cout << pluss::series::rand_btw(3.0, 3.0, {4}, 42) << "\n\n";

  heading("rand_btw_config_float32");
  std::cout << pluss::series::rand_btw(
      -2.0,
      2.0,
      {4},
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
      42) << "\n\n";

  heading("rand_btw_invalid_bounds");
  try {
    std::cout << pluss::series::rand_btw(2.0, -2.0, {4}, 42) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("normal_seeded");
  std::cout << pluss::series::normal(10.0, 2.0, {4}, pluss::ScalarType::Float64, 42) << "\n\n";

  heading("normal_matrix_seeded");
  std::cout << pluss::series::normal(0.0, 1.0, {2, 2}, pluss::ScalarType::Float32, 42) << "\n\n";

  heading("normal_zero_std");
  std::cout << pluss::series::normal(5.0, 0.0, {4}, pluss::ScalarType::Float64, 42) << "\n\n";

  heading("normal_negative_mean");
  std::cout << pluss::series::normal(-3.0, 0.5, {4}, pluss::ScalarType::Float64, 42) << "\n\n";

  heading("normal_same_seed_replay");
  auto normal_a = pluss::series::normal(1.0, 0.25, {4}, pluss::ScalarType::Float32, 5);
  auto normal_b = pluss::series::normal(1.0, 0.25, {4}, pluss::ScalarType::Float32, 5);
  std::cout << normal_a << "\n" << normal_b << "\n\n";

  heading("normal_invalid_std");
  try {
    std::cout << pluss::series::normal(0.0, -1.0, {4}, pluss::ScalarType::Float64, 42) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("randperm_seeded");
  std::cout << pluss::series::randperm(6, pluss::ScalarType::Int64, 42) << "\n\n";

  heading("randperm_empty");
  std::cout << pluss::series::randperm(0, pluss::ScalarType::Int64, 42) << "\n\n";

  heading("randperm_int32");
  std::cout << pluss::series::randperm(6, pluss::ScalarType::Int32, 42) << "\n\n";

  heading("randperm_same_seed_replay");
  auto randperm_a = pluss::series::randperm(6, pluss::ScalarType::Int64, 17);
  auto randperm_b = pluss::series::randperm(6, pluss::ScalarType::Int64, 17);
  std::cout << randperm_a << "\n" << randperm_b << "\n\n";

  heading("randperm_config_float32_normalizes");
  std::cout << pluss::series::randperm(
      6,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
      42) << "\n\n";

  heading("randperm_invalid_n");
  try {
    std::cout << pluss::series::randperm(-1, pluss::ScalarType::Int64, 42) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("empty_safe_init");
  auto scratch = pluss::series::empty({2, 2}, pluss::ScalarType::Float64);
  scratch.copy_(pluss::series::make({
      {5.0, 6.0},
      {7.0, 8.0},
  }));
  std::cout << scratch << "\n\n";

  heading("empty_int32_safe_init");
  auto ids = pluss::series::empty({3}, pluss::ScalarType::Int32);
  ids.copy_(pluss::series::make({10, 20, 30}, pluss::ScalarType::Int32));
  std::cout << ids << "\n\n";

  heading("empty_scalar_safe_init");
  auto scratch_scalar = pluss::series::empty({}, pluss::ScalarType::Float64);
  scratch_scalar.copy_(pluss::series::make(3.25));
  std::cout << scratch_scalar << "\n\n";

  heading("empty_config_bool_safe_init");
  auto mask = pluss::series::empty({2}, pluss::Config{}.with_dtype(pluss::ScalarType::Bool));
  mask.copy_(pluss::series::make({true, false}, pluss::ScalarType::Bool));
  std::cout << mask << "\n\n";

  heading("full_matrix");
  std::cout << pluss::series::full({2, 3}, 1.5) << "\n\n";

  heading("full_scalar");
  std::cout << pluss::series::full({}, -2.0) << "\n\n";

  heading("full_int32");
  std::cout << pluss::series::full({2, 2}, 7.0, pluss::ScalarType::Int32) << "\n\n";

  heading("full_bool_nonzero");
  std::cout << pluss::series::full({4}, 2.0, pluss::ScalarType::Bool) << "\n\n";

  heading("full_fraction_to_int");
  std::cout << pluss::series::full({3}, 2.75, pluss::ScalarType::Int32) << "\n\n";

  heading("ones_matrix");
  std::cout << pluss::series::ones({2, 3}) << "\n\n";

  heading("ones_int16");
  std::cout << pluss::series::ones({4}, pluss::ScalarType::Int16) << "\n\n";

  heading("ones_scalar");
  std::cout << pluss::series::ones({}, pluss::ScalarType::Float64) << "\n\n";

  heading("ones_bool");
  std::cout << pluss::series::ones({4}, pluss::ScalarType::Bool) << "\n\n";

  heading("zeros_matrix");
  std::cout << pluss::series::zeros({2, 3}) << "\n\n";

  heading("zeros_bool");
  std::cout << pluss::series::zeros({4}, pluss::ScalarType::Bool) << "\n\n";

  heading("zeros_scalar");
  std::cout << pluss::series::zeros({}, pluss::ScalarType::Float64) << "\n\n";

  heading("zeros_int16");
  std::cout << pluss::series::zeros({4}, pluss::ScalarType::Int16) << "\n\n";

  auto like_ref = pluss::series::make({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  auto like_scalar = pluss::series::make(5.0);
  auto like_rank3 = pluss::series::make(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {1, 2, 3});

  heading("empty_like_safe_init");
  auto empty_like_same = pluss::series::empty_like(like_ref);
  empty_like_same.copy_(pluss::series::make({
      {9.0, 8.0, 7.0},
      {6.0, 5.0, 4.0},
  }));
  std::cout << empty_like_same << "\n\n";

  heading("empty_like_dtype_override");
  auto empty_like_i16 = pluss::series::empty_like(like_ref, pluss::ScalarType::Int16);
  empty_like_i16.copy_(pluss::series::make({
      {9, 8, 7},
      {6, 5, 4},
  }, pluss::ScalarType::Int16));
  std::cout << empty_like_i16 << "\n\n";

  heading("empty_like_scalar_safe_init");
  auto empty_like_scalar = pluss::series::empty_like(like_scalar);
  empty_like_scalar.copy_(pluss::series::make(-9.5));
  std::cout << empty_like_scalar << "\n\n";

  heading("empty_like_config_bool_safe_init");
  auto empty_like_bool = pluss::series::empty_like(
      like_ref,
      pluss::Config{}.with_dtype(pluss::ScalarType::Bool));
  empty_like_bool.copy_(pluss::series::make({
      {true, false, true},
      {false, true, false},
  }, pluss::ScalarType::Bool));
  std::cout << empty_like_bool << "\n\n";

  heading("zeros_like_default");
  std::cout << pluss::series::zeros_like(like_ref) << "\n\n";

  heading("zeros_like_dtype_override");
  std::cout << pluss::series::zeros_like(like_ref, pluss::ScalarType::Int16) << "\n\n";

  heading("zeros_like_scalar");
  std::cout << pluss::series::zeros_like(like_scalar) << "\n\n";

  heading("zeros_like_config_bool");
  std::cout << pluss::series::zeros_like(
      like_ref,
      pluss::Config{}.with_dtype(pluss::ScalarType::Bool)) << "\n\n";

  heading("ones_like_default");
  std::cout << pluss::series::ones_like(like_ref) << "\n\n";

  heading("ones_like_dtype_override");
  std::cout << pluss::series::ones_like(like_ref, pluss::ScalarType::Bool) << "\n\n";

  heading("ones_like_scalar");
  std::cout << pluss::series::ones_like(like_scalar) << "\n\n";

  heading("ones_like_config_int16");
  std::cout << pluss::series::ones_like(
      like_ref,
      pluss::Config{}.with_dtype(pluss::ScalarType::Int16)) << "\n\n";

  heading("full_like_default");
  std::cout << pluss::series::full_like(like_ref, -2.5) << "\n\n";

  heading("full_like_dtype_override");
  std::cout << pluss::series::full_like(like_ref, 7.0, pluss::ScalarType::Int32) << "\n\n";

  heading("full_like_scalar");
  std::cout << pluss::series::full_like(like_scalar, -2.5) << "\n\n";

  heading("full_like_bool");
  std::cout << pluss::series::full_like(like_ref, 3.0, pluss::ScalarType::Bool) << "\n\n";

  heading("rand_like_seeded");
  std::cout << pluss::series::rand_like(like_ref, pluss::ScalarType::Float64, 42) << "\n\n";

  heading("rand_like_same_seed_replay");
  auto rand_like_a = pluss::series::rand_like(like_ref, pluss::ScalarType::Float32, 5);
  auto rand_like_b = pluss::series::rand_like(like_ref, pluss::ScalarType::Float32, 5);
  std::cout << rand_like_a << "\n" << rand_like_b << "\n\n";

  heading("rand_like_scalar_seeded");
  std::cout << pluss::series::rand_like(like_scalar, pluss::ScalarType::Float64, 42) << "\n\n";

  heading("rand_like_config_float32");
  std::cout << pluss::series::rand_like(
      like_ref,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
      42) << "\n\n";

  heading("randn_like_seeded");
  std::cout << pluss::series::randn_like(like_ref, pluss::ScalarType::Float64, 42) << "\n\n";

  heading("randn_like_same_seed_replay");
  auto randn_like_a = pluss::series::randn_like(like_ref, pluss::ScalarType::Float32, 5);
  auto randn_like_b = pluss::series::randn_like(like_ref, pluss::ScalarType::Float32, 5);
  std::cout << randn_like_a << "\n" << randn_like_b << "\n\n";

  heading("randn_like_scalar_seeded");
  std::cout << pluss::series::randn_like(like_scalar, pluss::ScalarType::Float64, 42) << "\n\n";

  heading("randn_like_config_float32");
  std::cout << pluss::series::randn_like(
      like_ref,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
      42) << "\n\n";

  heading("randint_like_hi_only");
  std::cout << pluss::series::randint_like(like_ref, 5, pluss::ScalarType::Int64, 42) << "\n\n";

  heading("randint_like_lo_hi");
  std::cout << pluss::series::randint_like(like_ref, 10, 15, pluss::ScalarType::Int32, 42) << "\n\n";

  heading("randint_like_config_float32_normalizes");
  std::cout << pluss::series::randint_like(
      like_ref,
      10,
      15,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
      42) << "\n\n";

  heading("randint_like_scalar_hi_only");
  std::cout << pluss::series::randint_like(like_scalar, 5, pluss::ScalarType::Int64, 42) << "\n\n";

  heading("randint_like_same_seed_replay");
  auto randint_like_a = pluss::series::randint_like(like_ref, 10, 15, pluss::ScalarType::Int32, 7);
  auto randint_like_b = pluss::series::randint_like(like_ref, 10, 15, pluss::ScalarType::Int32, 7);
  std::cout << randint_like_a << "\n" << randint_like_b << "\n\n";

  heading("randint_like_invalid_bounds");
  try {
    std::cout << pluss::series::randint_like(like_ref, 5, 5, pluss::ScalarType::Int64, 42) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("eye_like_default");
  std::cout << pluss::series::eye_like(like_ref) << "\n\n";

  heading("eye_like_bool");
  std::cout << pluss::series::eye_like(like_ref, pluss::ScalarType::Bool) << "\n\n";

  heading("eye_like_rank3_last_two_dims");
  std::cout << pluss::series::eye_like(like_rank3, pluss::ScalarType::Int16) << "\n\n";

  heading("eye_like_config_int16");
  std::cout << pluss::series::eye_like(
      like_ref,
      pluss::Config{}.with_dtype(pluss::ScalarType::Int16)) << "\n\n";

  heading("eye_like_invalid_rank");
  try {
    auto vec = pluss::series::make({1.0, 2.0, 3.0});
    std::cout << pluss::series::eye_like(vec) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("bernoulli_default_seeded");
  std::cout << pluss::series::bernoulli({8}, 0.5, pluss::ScalarType::Bool, 42) << "\n\n";

  heading("bernoulli_float32_cast");
  std::cout << pluss::series::bernoulli({8}, 0.25, pluss::ScalarType::Float32, 42) << "\n\n";

  heading("bernoulli_scalar_seeded");
  std::cout << pluss::series::bernoulli({}, 0.75, pluss::ScalarType::Bool, 42) << "\n\n";

  heading("bernoulli_matrix_seeded");
  std::cout << pluss::series::bernoulli({2, 3}, 0.4, pluss::ScalarType::Bool, 42) << "\n\n";

  heading("bernoulli_zero_prob");
  std::cout << pluss::series::bernoulli({6}, 0.0, pluss::ScalarType::Bool, 42) << "\n\n";

  heading("bernoulli_one_prob");
  std::cout << pluss::series::bernoulli({6}, 1.0, pluss::ScalarType::Bool, 42) << "\n\n";

  heading("bernoulli_config_int16");
  std::cout << pluss::series::bernoulli(
      {8},
      0.25,
      pluss::Config{}.with_dtype(pluss::ScalarType::Int16),
      42) << "\n\n";

  heading("bernoulli_same_seed_replay");
  auto bernoulli_a = pluss::series::bernoulli({8}, 0.5, pluss::ScalarType::Bool, 7);
  auto bernoulli_b = pluss::series::bernoulli({8}, 0.5, pluss::ScalarType::Bool, 7);
  std::cout << bernoulli_a << "\n" << bernoulli_b << "\n\n";

  heading("bernoulli_invalid_p");
  try {
    std::cout << pluss::series::bernoulli({4}, 1.5, pluss::ScalarType::Bool, 42) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("poisson_seeded");
  std::cout << pluss::series::poisson({8}, 3.0, pluss::ScalarType::Int64, 42) << "\n\n";

  heading("poisson_zero_lambda");
  std::cout << pluss::series::poisson({6}, 0.0, pluss::ScalarType::Int64, 42) << "\n\n";

  heading("poisson_scalar_seeded");
  std::cout << pluss::series::poisson({}, 3.0, pluss::ScalarType::Int64, 42) << "\n\n";

  heading("poisson_matrix_seeded");
  std::cout << pluss::series::poisson({2, 3}, 2.0, pluss::ScalarType::Int32, 42) << "\n\n";

  heading("poisson_small_lambda");
  std::cout << pluss::series::poisson({8}, 0.25, pluss::ScalarType::Int64, 42) << "\n\n";

  heading("poisson_config_float32_normalizes");
  std::cout << pluss::series::poisson(
      {6},
      2.0,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
      42) << "\n\n";

  heading("poisson_same_seed_replay");
  auto poisson_a = pluss::series::poisson({6}, 2.0, pluss::ScalarType::Int64, 7);
  auto poisson_b = pluss::series::poisson({6}, 2.0, pluss::ScalarType::Int64, 7);
  std::cout << poisson_a << "\n" << poisson_b << "\n\n";

  heading("poisson_invalid_lambda");
  try {
    std::cout << pluss::series::poisson({4}, -1.0, pluss::ScalarType::Int64, 42) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  const std::vector<double> probs = {0.1, 0.2, 0.7};
  const std::span<const double> probs_span(probs.data(), probs.size());

  heading("multinomial_replacement");
  std::cout << pluss::series::multinomial(probs_span, 6, true, 42) << "\n\n";

  heading("multinomial_without_replacement");
  std::cout << pluss::series::multinomial(probs_span, 3, false, 42) << "\n\n";

  heading("multinomial_same_seed_replay");
  auto multinomial_a = pluss::series::multinomial(probs_span, 6, true, 11);
  auto multinomial_b = pluss::series::multinomial(probs_span, 6, true, 11);
  std::cout << multinomial_a << "\n" << multinomial_b << "\n\n";

  heading("multinomial_single_sample");
  std::cout << pluss::series::multinomial(probs_span, 1, true, 42) << "\n\n";

  heading("multinomial_zero_prob_bucket");
  {
    const std::vector<double> zero_bucket_probs = {0.0, 1.0, 0.0};
    const std::span<const double> zero_bucket_span(zero_bucket_probs.data(), zero_bucket_probs.size());
    std::cout << pluss::series::multinomial(zero_bucket_span, 4, true, 42) << "\n\n";
  }

  heading("multinomial_config_int32");
  std::cout << pluss::series::multinomial(
      probs_span,
      6,
      true,
      pluss::Config{}.with_dtype(pluss::ScalarType::Int32),
      42) << "\n\n";

  heading("multinomial_empty_probs");
  try {
    const std::vector<double> empty_probs;
    const std::span<const double> empty_span(empty_probs.data(), empty_probs.size());
    std::cout << pluss::series::multinomial(empty_span, 3, true, 42) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("multinomial_insufficient_mass");
  try {
    const std::vector<double> sparse_probs = {0.7, 0.0, 0.3};
    const std::span<const double> sparse_span(sparse_probs.data(), sparse_probs.size());
    std::cout << pluss::series::multinomial(sparse_span, 3, false, 42) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("multinomial_negative_samples");
  try {
    std::cout << pluss::series::multinomial(probs_span, -1, true, 42) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  auto printable = pluss::series::make({
      {1.25, 2.5, 3.75},
      {4.0, 5.5, 6.25},
  });

  heading("print_default");
  std::cout << pluss::series::print(printable) << "\n\n";

  heading("print_stream_overload");
  pluss::series::print(std::cout, printable) << "\n\n";

  heading("print_values_only");
  {
    pluss::PrintOptions opt;
    opt.metadata_mode = pluss::codem::PrintMetadataMode::None;
    std::cout << pluss::series::print(printable, opt) << "\n\n";
  }

  heading("print_bool_one_zero");
  {
    pluss::PrintOptions opt;
    opt.bool_style = pluss::codem::PrintBoolStyle::OneZero;
    std::cout << pluss::series::print(
        pluss::series::make({true, false, true, true}, pluss::ScalarType::Bool),
        opt) << "\n\n";
  }

  heading("select_single_branch");
  std::cout << pluss::series::select(
      pluss::series::make({true, false, true, false}, pluss::ScalarType::Bool),
      pluss::series::make({10.0, 20.0, 30.0, 40.0}),
      -1.0) << "\n\n";

  heading("select_first_match_wins");
  std::cout << pluss::series::select(
      {
          pluss::series::make({true, true, false, false}, pluss::ScalarType::Bool),
          pluss::series::make({true, false, true, false}, pluss::ScalarType::Bool),
      },
      {
          pluss::series::make({100.0, 100.0, 100.0, 100.0}),
          pluss::series::make({200.0, 200.0, 200.0, 200.0}),
      },
      0.0) << "\n\n";

  heading("select_series_default");
  std::cout << pluss::series::select(
      {
          pluss::series::make({false, true, false, false}, pluss::ScalarType::Bool),
          pluss::series::make({false, false, true, false}, pluss::ScalarType::Bool),
      },
      {
          pluss::series::make({10.0, 10.0, 10.0, 10.0}),
          pluss::series::make({20.0, 20.0, 20.0, 20.0}),
      },
      pluss::series::make({-1.0, -2.0, -3.0, -4.0})) << "\n\n";

  heading("select_scalar_choice_broadcast");
  std::cout << pluss::series::select(
      {
          pluss::series::make({false, true, false, true}, pluss::ScalarType::Bool),
      },
      {
          pluss::series::make(9.0),
      },
      pluss::series::make({1.0, 2.0, 3.0, 4.0})) << "\n\n";

  heading("select_bool_output");
  std::cout << pluss::series::select(
      {
          pluss::series::make({true, false, false, true}, pluss::ScalarType::Bool),
          pluss::series::make({false, true, false, false}, pluss::ScalarType::Bool),
      },
      {
          pluss::series::make({true, true, true, true}, pluss::ScalarType::Bool),
          pluss::series::make({false, false, false, false}, pluss::ScalarType::Bool),
      },
      true) << "\n\n";

  heading("select_single_series_overload");
  std::cout << pluss::series::select(
      pluss::series::make({false, true, false, false}, pluss::ScalarType::Bool),
      pluss::series::make(5.0),
      1.0) << "\n\n";

  heading("select_condition_choice_mismatch");
  try {
    std::cout << pluss::series::select(
        {
            pluss::series::make({true, false}, pluss::ScalarType::Bool),
            pluss::series::make({false, true}, pluss::ScalarType::Bool),
        },
        {
            pluss::series::make({1.0, 1.0}),
        },
        0.0) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("select_non_bool_condition");
  try {
    std::cout << pluss::series::select(
        pluss::series::make({1.0, 0.0, 1.0}),
        pluss::series::make({10.0, 20.0, 30.0}),
        -1.0) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  auto real32 = pluss::series::make({
      {1.0f, 2.0f},
      {3.0f, 4.0f},
  }, pluss::ScalarType::Float32);
  auto imag32 = pluss::series::make({
      {0.5f, 1.5f},
      {2.5f, 3.5f},
  }, pluss::ScalarType::Float32);

  heading("complex_complex64");
  std::cout << pluss::series::complex(real32, imag32, pluss::ScalarType::Complex64) << "\n\n";

  auto real64 = pluss::series::make({
      {1.0, 2.0},
      {3.0, 4.0},
  }, pluss::ScalarType::Float64);
  auto imag64 = pluss::series::make({
      {0.25, 0.5},
      {0.75, 1.0},
  }, pluss::ScalarType::Float64);

  heading("complex_complex128");
  std::cout << pluss::series::complex(real64, imag64, pluss::ScalarType::Complex128) << "\n\n";

  heading("complex_scalar_complex64");
  std::cout << pluss::series::complex(
      pluss::series::make(2.0),
      pluss::series::make(-0.5),
      pluss::ScalarType::Complex64) << "\n\n";

  heading("complex_integer_inputs");
  std::cout << pluss::series::complex(
      pluss::series::make({1, 2, 3}, pluss::ScalarType::Int32),
      pluss::series::make({4, 5, 6}, pluss::ScalarType::Int32),
      pluss::ScalarType::Complex128) << "\n\n";

  heading("complex_shape_mismatch");
  try {
    auto bad_imag = pluss::series::make({1.0, 2.0});
    std::cout << pluss::series::complex(real64, bad_imag, pluss::ScalarType::Complex128) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("complex_invalid_dtype");
  try {
    std::cout << pluss::series::complex(real64, imag64, pluss::ScalarType::Float32) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  auto magnitudes = pluss::series::make({1.0, 2.0}, pluss::ScalarType::Float64);
  auto angles = pluss::series::make({0.0, 1.5707963267948966}, pluss::ScalarType::Float64);

  heading("polar_complex64");
  std::cout << pluss::series::polar(magnitudes, angles, pluss::ScalarType::Complex64) << "\n\n";

  heading("polar_complex128");
  std::cout << pluss::series::polar(magnitudes, angles, pluss::ScalarType::Complex128) << "\n\n";

  heading("polar_scalar_complex64");
  std::cout << pluss::series::polar(
      pluss::series::make(2.0),
      pluss::series::make(3.14159265358979323846),
      pluss::ScalarType::Complex64) << "\n\n";

  heading("polar_matrix_complex64");
  std::cout << pluss::series::polar(
      pluss::series::make({
          {1.0, 2.0},
          {3.0, 4.0},
      }),
      pluss::series::make({
          {0.0, 1.5707963267948966},
          {3.14159265358979323846, 4.7123889803846898577},
      }),
      pluss::ScalarType::Complex64) << "\n\n";

  heading("polar_zero_magnitude");
  std::cout << pluss::series::polar(
      pluss::series::make({0.0, 0.0}),
      pluss::series::make({0.0, 1.0}),
      pluss::ScalarType::Complex64) << "\n\n";

  heading("polar_invalid_dtype");
  try {
    std::cout << pluss::series::polar(magnitudes, angles, pluss::ScalarType::Float32) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  heading("polar_shape_mismatch");
  try {
    auto bad_angles = pluss::series::make({
        {0.0, 1.0},
        {2.0, 3.0},
    });
    std::cout << pluss::series::polar(magnitudes, bad_angles, pluss::ScalarType::Complex64) << "\n";
  } catch (const std::exception& e) {
    std::string msg = e.what();
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  std::cout << "\n";

  return 0;
}
