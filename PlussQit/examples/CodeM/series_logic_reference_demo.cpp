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

  return 0;
}
