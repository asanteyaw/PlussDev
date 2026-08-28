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
  const auto a = pluss::make_series({3, 1, 3, 2}, pluss::ScalarType::Int64);
  const auto b = pluss::make_series({2, 4, 1}, pluss::ScalarType::Int64);
  const auto c = pluss::make_series({4, 2, 9}, pluss::ScalarType::Int64);

  heading("union1d_sorted_default");
  pluss::series::print(std::cout, a.union1d(b)) << "\n\n";

  heading("union1d_preserve_first_seen");
  std::cout << a.union1d(b, false) << "\n\n";

  heading("union1d_flattens_inputs");
  std::cout << pluss::make_series({
      {1, 2},
      {2, 3},
  }, pluss::ScalarType::Int64).union1d(
      pluss::make_series({
          {3, 4},
          {1, 5},
      }, pluss::ScalarType::Int64),
      false) << "\n\n";

  heading("union1d_empty_left");
  std::cout << pluss::series::zeros({0}, pluss::ScalarType::Int64).union1d(
      pluss::make_series({2, 2, 3}, pluss::ScalarType::Int64),
      false) << "\n\n";

  heading("union1d_dtype_error");
  try {
    std::cout << pluss::make_series({1.0, 2.0}).union1d(b, false) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("unionize_empty_others");
  pluss::series::print(std::cout, a.unionize({}, false)) << "\n\n";

  heading("unionize_many_preserve_first_seen");
  std::cout << a.unionize({b, c}, false) << "\n\n";

  heading("unionize_many_sorted");
  std::cout << a.unionize({b, c}) << "\n\n";

  heading("unionize_chain_order");
  std::cout << pluss::make_series({5, 1}, pluss::ScalarType::Int64).unionize({
      pluss::make_series({1, 2}, pluss::ScalarType::Int64),
      pluss::make_series({2, 5, 3}, pluss::ScalarType::Int64),
      pluss::make_series({3, 4}, pluss::ScalarType::Int64),
  }, false) << "\n\n";

  heading("intersect1d_sorted_default");
  pluss::series::print(std::cout, a.intersect1d(b)) << "\n\n";

  heading("intersect1d_preserve_left_order");
  std::cout << pluss::make_series({5, 2, 5, 1, 2}, pluss::ScalarType::Int64).intersect1d(
      pluss::make_series({2, 1}, pluss::ScalarType::Int64),
      false) << "\n\n";

  heading("intersect1d_assume_unique_keeps_duplicates");
  std::cout << pluss::make_series({2, 2, 1}, pluss::ScalarType::Int64).intersect1d(
      pluss::make_series({1, 2}, pluss::ScalarType::Int64),
      false,
      true) << "\n\n";

  heading("intersect1d_empty_result");
  std::cout << pluss::make_series({8, 9}, pluss::ScalarType::Int64).intersect1d(
      pluss::make_series({1, 2}, pluss::ScalarType::Int64),
      false) << "\n\n";

  heading("intersection_empty_others");
  pluss::series::print(std::cout, a.intersection({}, false, false)) << "\n\n";

  heading("intersection_many_basic");
  std::cout << a.intersection({b, c}, false, false) << "\n\n";

  heading("intersection_many_preserve_left_order");
  std::cout << pluss::make_series({5, 2, 1, 3, 2}, pluss::ScalarType::Int64).intersection({
      pluss::make_series({2, 5, 9}, pluss::ScalarType::Int64),
      pluss::make_series({5, 2, 7}, pluss::ScalarType::Int64),
  }, false, false) << "\n\n";

  heading("intersection_assume_unique_keeps_duplicates");
  std::cout << pluss::make_series({2, 2, 1}, pluss::ScalarType::Int64).intersection({
      pluss::make_series({2, 1}, pluss::ScalarType::Int64),
      pluss::make_series({2}, pluss::ScalarType::Int64),
  }, false, true) << "\n\n";

  heading("intersection_short_circuits_to_empty");
  std::cout << pluss::make_series({5, 2, 1}, pluss::ScalarType::Int64).intersection({
      pluss::make_series({2, 5}, pluss::ScalarType::Int64),
      pluss::make_series({7, 8}, pluss::ScalarType::Int64),
  }, false, false) << "\n\n";

  heading("setdiff1d_sorted_default");
  pluss::series::print(std::cout, a.setdiff1d(b)) << "\n\n";

  heading("setdiff1d_preserve_left_order");
  std::cout << pluss::make_series({5, 2, 5, 1, 3}, pluss::ScalarType::Int64).setdiff1d(
      pluss::make_series({2, 1}, pluss::ScalarType::Int64),
      false) << "\n\n";

  heading("setdiff1d_assume_unique_keeps_duplicates");
  std::cout << pluss::make_series({5, 5, 3, 2}, pluss::ScalarType::Int64).setdiff1d(
      pluss::make_series({2}, pluss::ScalarType::Int64),
      false,
      true) << "\n\n";

  heading("setdiff1d_rhs_empty");
  std::cout << pluss::make_series({2, 2, 3}, pluss::ScalarType::Int64).setdiff1d(
      pluss::series::zeros({0}, pluss::ScalarType::Int64),
      false) << "\n\n";

  heading("setxor1d_sorted_default");
  pluss::series::print(std::cout, a.setxor1d(b)) << "\n\n";

  heading("setxor1d_preserve_side_order");
  std::cout << a.setxor1d(b, false) << "\n\n";

  heading("setxor1d_assume_unique_keeps_duplicates");
  std::cout << pluss::make_series({5, 5, 2}, pluss::ScalarType::Int64).setxor1d(
      pluss::make_series({2, 7, 7}, pluss::ScalarType::Int64),
      false,
      true) << "\n\n";

  heading("setxor1d_with_empty");
  std::cout << pluss::series::zeros({0}, pluss::ScalarType::Int64).setxor1d(
      pluss::make_series({2, 2, 3}, pluss::ScalarType::Int64),
      false) << "\n\n";

  const auto along_left = pluss::make_series({
      {1, 2, 3},
      {4, 5, 6},
  }, pluss::ScalarType::Int64);
  const auto along_right_dim0 = pluss::make_series({
      {4, 5, 6},
      {7, 8, 9},
  }, pluss::ScalarType::Int64);
  const auto along_right_dim1 = pluss::make_series({
      {2, 7, 8},
      {5, 9, 10},
  }, pluss::ScalarType::Int64);
  const auto nd_along_left =
      pluss::make_series({1, 2, 3, 4, 5, 6}, pluss::ScalarType::Int64).reshape({1, 2, 3});
  const auto nd_along_right =
      pluss::make_series({3, 2, 7, 4, 5, 6}, pluss::ScalarType::Int64).reshape({1, 2, 3});

  heading("unionize_along_dim0");
  pluss::series::print(std::cout, along_left.unionize_along(along_right_dim0, 0, false)) << "\n\n";

  heading("unionize_along_dim1");
  std::cout << along_left.unionize_along(along_right_dim1, 1, false) << "\n\n";

  heading("unionize_along_negative_dim");
  std::cout << nd_along_left.unionize_along(nd_along_right, -1, false) << "\n\n";

  heading("unionize_along_sorted_rows");
  std::cout << pluss::make_series({
      {4, 5, 6},
      {1, 2, 3},
  }, pluss::ScalarType::Int64).unionize_along(
      pluss::make_series({
          {7, 8, 9},
          {4, 5, 6},
      }, pluss::ScalarType::Int64),
      0,
      true) << "\n\n";

  heading("unionize_along_shape_error");
  try {
    std::cout << along_left.unionize_along(
        pluss::make_series({
            {1, 2},
            {3, 4},
        }, pluss::ScalarType::Int64),
        0,
        false) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("unionize_along_dim_error");
  try {
    std::cout << along_left.unionize_along(along_right_dim0, 3, false) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("intersection_along_dim0");
  pluss::series::print(std::cout, along_left.intersection_along(along_right_dim0, 0, false, false)) << "\n\n";

  heading("intersection_along_dim1");
  std::cout << along_left.intersection_along(along_right_dim1, 1, false, false) << "\n\n";

  heading("intersection_along_negative_dim");
  std::cout << nd_along_left.intersection_along(nd_along_right, 2, false, false) << "\n\n";

  heading("intersection_along_assume_unique_keeps_duplicate_slices");
  std::cout << pluss::make_series({
      {1, 2},
      {1, 2},
      {3, 4},
  }, pluss::ScalarType::Int64).intersection_along(
      pluss::make_series({
          {1, 2},
          {9, 9},
      }, pluss::ScalarType::Int64),
      0,
      false,
      true) << "\n\n";

  heading("setdiff_along_dim0");
  pluss::series::print(std::cout, along_left.setdiff_along(along_right_dim0, 0, false, false)) << "\n\n";

  heading("setdiff_along_dim1");
  std::cout << along_left.setdiff_along(along_right_dim1, 1, false, false) << "\n\n";

  heading("setdiff_along_negative_dim");
  std::cout << nd_along_left.setdiff_along(nd_along_right, 2, false, false) << "\n\n";

  heading("setdiff_along_assume_unique_keeps_duplicate_slices");
  std::cout << pluss::make_series({
      {1, 2},
      {1, 2},
      {3, 4},
  }, pluss::ScalarType::Int64).setdiff_along(
      pluss::make_series({
          {3, 4},
      }, pluss::ScalarType::Int64),
      0,
      false,
      true) << "\n\n";

  heading("setxor_along_dim0");
  pluss::series::print(std::cout, along_left.setxor_along(along_right_dim0, 0, false, false)) << "\n\n";

  heading("setxor_along_dim1");
  std::cout << along_left.setxor_along(along_right_dim1, 1, false, false) << "\n\n";

  heading("setxor_along_negative_dim");
  std::cout << nd_along_left.setxor_along(nd_along_right, 2, false, false) << "\n\n";

  heading("setxor_along_assume_unique_keeps_duplicate_slices");
  std::cout << pluss::make_series({
      {1, 2},
      {1, 2},
      {3, 4},
  }, pluss::ScalarType::Int64).setxor_along(
      pluss::make_series({
          {3, 4},
          {9, 9},
          {9, 9},
      }, pluss::ScalarType::Int64),
      0,
      false,
      true) << "\n\n";

  heading("comp_basic");
  pluss::series::print(std::cout, pluss::make_series({1, 2, 2, 4}, pluss::ScalarType::Int64).comp(
      pluss::make_series({0, 1, 2, 3, 4}, pluss::ScalarType::Int64))) << "\n\n";

  heading("comp_preserve_universe_order");
  std::cout << pluss::make_series({1, 4}, pluss::ScalarType::Int64).comp(
      pluss::make_series({7, 4, 0, 1, 3}, pluss::ScalarType::Int64),
      false) << "\n\n";

  heading("comp_assume_unique_keeps_duplicate_universe_values");
  std::cout << pluss::make_series({1, 2, 4}, pluss::ScalarType::Int64).comp(
      pluss::make_series({0, 0, 1, 2, 3, 3, 4}, pluss::ScalarType::Int64),
      false,
      true) << "\n\n";

  heading("comp_ignores_values_outside_universe");
  std::cout << pluss::make_series({1, 2, 9}, pluss::ScalarType::Int64).comp(
      pluss::make_series({0, 1, 2, 3}, pluss::ScalarType::Int64),
      false) << "\n\n";

  heading("cartprod_pair");
  pluss::series::print(std::cout, pluss::make_series({1, 2}, pluss::ScalarType::Int64).cartprod(
      pluss::make_series({3, 4, 5}, pluss::ScalarType::Int64))) << "\n\n";

  heading("cartprod_many");
  std::cout << pluss::make_series({1, 2}, pluss::ScalarType::Int64).cartprod({
      pluss::make_series({3, 4, 5}, pluss::ScalarType::Int64),
      pluss::make_series({9}, pluss::ScalarType::Int64),
  }) << "\n\n";

  heading("cartprod_flattens_inputs");
  pluss::PrintOptions full;
  full.max_elements = 32;
  full.edge_items = 32;
  pluss::series::print(std::cout, pluss::make_series({
      {1, 2},
      {3, 4},
  }, pluss::ScalarType::Int64).cartprod(
      pluss::make_series({8, 9}, pluss::ScalarType::Int64).reshape({2, 1})), full) << "\n\n";

  heading("cartprod_empty_operand");
  std::cout << pluss::series::zeros({0}, pluss::ScalarType::Int64).cartprod(
      pluss::make_series({3, 4}, pluss::ScalarType::Int64)) << "\n\n";

  heading("cartprod_dtype_error");
  try {
    std::cout << pluss::make_series({1, 2}, pluss::ScalarType::Int64).cartprod(
        pluss::make_series({1.0, 2.0})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}
