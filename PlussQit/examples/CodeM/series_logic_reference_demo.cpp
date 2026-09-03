#include <pluss/pluss.h>

#include <iostream>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error(const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

} // namespace

int main() {
  const auto truth = pluss::make_series({
      {1.0, 1.0, 0.0},
      {2.0, -3.0, 4.0},
  });
  const auto sparse = pluss::make_series({
      {0.0, 0.0, 0.0},
      {0.0, 5.0, 0.0},
  });

  heading("all_global_numeric");
  pluss::series::print(std::cout, truth.all()) << "\n\n";

  heading("all_dim0");
  std::cout << truth.all(0, false) << "\n\n";

  heading("all_dim1_keepdim");
  std::cout << truth.all(1, true) << "\n\n";

  heading("all_negative_dim");
  std::cout << truth.all(-1, false) << "\n\n";

  heading("all_scalar_zero");
  std::cout << pluss::make_series(0.0).all() << "\n\n";

  heading("all_bad_dim");
  try {
    std::cout << truth.all(2, false) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("any_global_numeric");
  pluss::series::print(std::cout, sparse.any()) << "\n\n";

  heading("any_dim0");
  std::cout << sparse.any(0, false) << "\n\n";

  heading("any_dim1_keepdim");
  std::cout << sparse.any(1, true) << "\n\n";

  heading("any_negative_dim");
  std::cout << sparse.any(-1, false) << "\n\n";

  heading("any_scalar_zero");
  std::cout << pluss::make_series(0.0).any() << "\n\n";

  heading("any_bad_dim");
  try {
    std::cout << sparse.any(2, false) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("where_scalar_other");
  auto self = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  auto cond = pluss::make_series({false, true, false, true});
  std::cout << self.where(cond, -1.0) << "\n\n";

  heading("logical_not_inplace_numeric");
  auto numeric = pluss::make_series({0.0, 2.0, 0.0, -5.0});
  numeric.logical_not_();
  pluss::series::print(std::cout, numeric) << "\n\n";

  heading("where_series_other");
  auto other = pluss::make_series({100.0, 200.0, 300.0, 400.0});
  std::cout << self.where(cond, other) << "\n\n";

  heading("where_broadcast_condition");
  auto matrix = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  auto col_mask = pluss::make_series({true, false, true});
  std::cout << matrix.where(col_mask, -1.0) << "\n\n";

  heading("where_bool_self_style");
  auto mask = pluss::make_series({true, false, true, false});
  auto a = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  auto b = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  std::cout << mask.where(a, b) << "\n\n";

  heading("where_numeric_condition_cast");
  auto numeric_cond = pluss::make_series({0.0, 2.0, 0.0, -5.0});
  std::cout << self.where(numeric_cond, other) << "\n\n";

  heading("where_type_promotion");
  auto ints = pluss::make_series({1, 2, 3}, pluss::ScalarType::Int32);
  auto halfs = pluss::make_series({0.5, 0.5, 0.5}).to(pluss::ScalarType::Float32);
  auto pick = pluss::make_series({true, false, true});
  std::cout << ints.where(pick, halfs) << "\n\n";

  heading("where_gradient");
  auto x = pluss::make_series({1.0, 2.0, 3.0});
  auto y = pluss::make_series({10.0, 20.0, 30.0});
  auto grad_mask = pluss::make_series({true, false, true});
  x.tracks_slope_(true);
  y.tracks_slope_(true);
  auto out = x.where(grad_mask, y);
  out.sum().reverse();
  std::cout << "out=" << out << "\n";
  std::cout << "dx=" << x.sensitivity() << "\n";
  std::cout << "dy=" << y.sensitivity() << "\n\n";

  heading("where_broadcast_error");
  try {
    std::cout << matrix.where(pluss::make_series({true, false}), -1.0) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto bits_left = pluss::make_series({1, 6, 12, 10}, pluss::ScalarType::Int32);
  auto bits_right = pluss::make_series({3, 10, 5, 12}, pluss::ScalarType::Int32);

  heading("bitwise_and_integer");
  std::cout << bits_left.bitwise_and(bits_right) << "\n\n";

  heading("bitwise_or_integer");
  std::cout << bits_left.bitwise_or(bits_right) << "\n\n";

  heading("bitwise_xor_integer");
  std::cout << bits_left.bitwise_xor(bits_right) << "\n\n";

  heading("bitwise_not_integer");
  pluss::series::print(std::cout, bits_left.bitwise_not()) << "\n\n";

  heading("bitwise_bool");
  auto bool_left = pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool);
  auto bool_right = pluss::make_series({true, true, false, false}, pluss::ScalarType::Bool);
  std::cout << bool_left.bitwise_and(bool_right) << "\n";
  std::cout << bool_left.bitwise_or(bool_right) << "\n";
  std::cout << bool_left.bitwise_xor(bool_right) << "\n\n";

  heading("bitwise_float_error");
  try {
    std::cout << pluss::make_series({1.0, 2.0}).bitwise_and(
        pluss::make_series({1.0, 0.0})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto truth_left = pluss::make_series({0, 2, 0, -5}, pluss::ScalarType::Int32);
  auto truth_right = pluss::make_series({1, 0, 3, 0}, pluss::ScalarType::Int32);

  heading("logical_numeric_truthiness");
  std::cout << truth_left.logical_and(truth_right) << "\n";
  std::cout << truth_left.logical_or(truth_right) << "\n";
  std::cout << truth_left.logical_xor(truth_right) << "\n\n";

  heading("logical_and_broadcast");
  auto truth_matrix = pluss::make_series({
      {true, false},
      {true, false},
  }, pluss::ScalarType::Bool);
  std::cout << truth_matrix.logical_and(
      pluss::make_series({true, false}, pluss::ScalarType::Bool)) << "\n\n";

  heading("logical_or_broadcast");
  std::cout << truth_matrix.logical_or(
      pluss::make_series({false, true}, pluss::ScalarType::Bool)) << "\n\n";

  heading("logical_not_numeric");
  pluss::series::print(std::cout, truth_left.logical_not()) << "\n\n";

  heading("logical_not_out_of_place_preserves_source");
  auto original_numeric = pluss::make_series({0.0, 2.0, 0.0, -5.0});
  auto inverted_numeric = original_numeric.logical_not();
  std::cout << original_numeric << "\n";
  std::cout << inverted_numeric << "\n\n";

  heading("logical_not_inplace_bool");
  auto inplace_mask = pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool);
  auto& inplace_result = inplace_mask.logical_not_();
  std::cout << inplace_result << "\n";
  std::cout << inplace_mask << "\n\n";

  heading("logical_broadcast_error");
  try {
    std::cout << truth_matrix.logical_or(
        pluss::make_series({true, false, true}, pluss::ScalarType::Bool)) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
