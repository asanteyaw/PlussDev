#include <pluss/pluss.h>

#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void show(const pluss::Series& value) {
  pluss::series::print(std::cout, value) << "\n\n";
}

void show_error(const std::exception& error) {
  const std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n\n";
}

} // namespace

int main() {
  heading("airy_landmarks");
  show(pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).airy());

  heading("airy_shape_and_promotion");
  show(pluss::make_series({{-1, 0}, {1, 2}}).airy());

  heading("airy_reverse");
  auto airy_x = pluss::make_series({-1.0, 0.0, 1.0});
  airy_x.tracks_slope_(true);
  airy_x.airy().sum().reverse();
  show(airy_x.sensitivity());

  heading("airy_scalar");
  const auto airy_scalar = pluss::make_series(0.0).airy();
  std::cout << "dim=" << airy_scalar.dim()
            << " value=" << airy_scalar.item<double>() << "\n\n";

  heading("as_strided_transpose");
  const auto base = pluss::series::arange(0.0, 6.0, 1.0, pluss::ScalarType::Float64);
  show(base.as_strided({3, 2}, {1, 3}));

  heading("as_strided_overlap");
  show(base.as_strided({3, 3}, {1, 1}));

  heading("as_strided_offset");
  show(base.as_strided({2, 2}, {2, 1}, 1));

  heading("as_strided_reverse_overlap");
  auto stride_x = pluss::series::arange(0.0, 5.0, 1.0, pluss::ScalarType::Float64);
  stride_x.tracks_slope_(true);
  stride_x.as_strided({3, 3}, {1, 1}).sum().reverse();
  show(stride_x.sensitivity());

  heading("as_strided_rank_error");
  try {
    (void)base.as_strided({2, 2}, {1});
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("as_strided_bounds_error");
  try {
    (void)base.as_strided({4, 4}, {2, 1});
  } catch (const std::exception& error) {
    show_error(error);
  }

  const auto x = pluss::make_series({0.5, 1.0, 2.0, 4.0});
  heading("bessel_order_zero");
  show(x.besselj());
  show(x.bessely());
  show(x.besseli());
  show(x.besselk());

  heading("bessel_orders");
  show(x.besselj(1.0));
  show(x.bessely(1.0));
  show(x.besseli(1.0));
  show(x.besselk(1.0));
  show(x.besselj(0.5));

  heading("bessel_domains");
  show(pluss::make_series({-1.0, 0.0, 1.0}).bessely());
  show(pluss::make_series({-1.0, 0.0, 1.0}).besselk());

  heading("besseli_parity");
  const auto parity_x = pluss::make_series({-1.0, 0.0, 1.0});
  show(parity_x.besseli());
  show(parity_x.besseli(1.0));

  heading("besseli_scalar");
  const auto bi_scalar = pluss::make_series(0.0).besseli();
  std::cout << "dim=" << bi_scalar.dim()
            << " value=" << bi_scalar.item<double>() << "\n\n";

  heading("besselk_order_symmetry");
  std::cout << std::boolalpha
            << x.besselk(-1.5).allclose(x.besselk(1.5)) << "\n\n";

  heading("bessely_matrix_shape");
  const auto by_matrix = pluss::make_series({{0.5, 1.0}, {2.0, 4.0}}).bessely();
  std::cout << "dim=" << by_matrix.dim()
            << " shape=[" << by_matrix.size(0) << "," << by_matrix.size(1) << "]\n\n";

  heading("bessel_reverse");
  auto bessel_x = pluss::make_series({0.5, 1.0, 2.0});
  bessel_x.tracks_slope_(true);
  bessel_x.besselj().sum().reverse();
  show(bessel_x.sensitivity());

  auto by_x = pluss::make_series({0.5, 1.0, 2.0});
  by_x.tracks_slope_(true);
  by_x.bessely().sum().reverse();
  show(by_x.sensitivity());

  auto bi_x = pluss::make_series({0.5, 1.0, 2.0});
  bi_x.tracks_slope_(true);
  bi_x.besseli().sum().reverse();
  show(bi_x.sensitivity());

  auto bk_x = pluss::make_series({0.5, 1.0, 2.0});
  bk_x.tracks_slope_(true);
  bk_x.besselk().sum().reverse();
  show(bk_x.sensitivity());

  heading("bessel_order_error");
  try {
    (void)x.besselj(std::numeric_limits<double>::infinity());
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("celu_default");
  show(pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).celu());

  heading("celu_alpha");
  show(pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).celu(2.0));

  heading("celu_integer_promotion");
  show(pluss::make_series({{-2, 0}, {1, 3}}).celu(0.5));

  heading("celu_reverse");
  auto celu_x = pluss::make_series({-2.0, -1.0, 0.0, 1.0});
  celu_x.tracks_slope_(true);
  celu_x.celu(2.0).sum().reverse();
  show(celu_x.sensitivity());

  heading("celu_zero_error");
  try {
    (void)x.celu(0.0);
  } catch (const std::exception& error) {
    show_error(error);
  }
}
