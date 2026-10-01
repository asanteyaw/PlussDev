#include <pluss/pluss.h>

#include <complex>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

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

void show_meta(const pluss::Series& value) {
  std::cout << "dtype=" << value.scalar_type() << " dim=" << value.dim() << " shape=[";
  for (std::int64_t i = 0; i < value.dim(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << value.size(i);
  }
  std::cout << "]\n\n";
}

}  // namespace

int main() {
  using Complex = std::complex<double>;

  heading("cplxpair_pairs_and_reals");
  show(pluss::make_series(std::vector<Complex>{
      {3.0, 0.0}, {2.0, 4.0}, {1.0, -2.0}, {2.0, -4.0},
      {-1.0, 0.0}, {1.0, 2.0}}, {6}).cplxpair());

  heading("cplxpair_real_matrix");
  show(pluss::make_series({{4, 1, 3}, {0, -2, 8}}).cplxpair());

  heading("cplxpair_shape");
  show_meta(pluss::make_series(std::vector<Complex>{
      {2.0, 1.0}, {2.0, -1.0}, {1.0, 3.0}, {1.0, -3.0}}, {2, 2}).cplxpair());

  heading("cplxpair_reverse_permutation");
  auto pair_x = pluss::make_series(std::vector<Complex>{
      {2.0, 1.0}, {1.0, -3.0}, {2.0, -1.0}, {1.0, 3.0}}, {4});
  pair_x.tracks_slope_(true);
  pair_x.cplxpair().real().mul(pluss::make_series({1.0, 2.0, 3.0, 4.0})).sum().reverse();
  show(pair_x.sensitivity());

  heading("cplxpair_unmatched_error");
  try {
    (void)pluss::make_series(std::vector<Complex>{{1.0, 2.0}, {3.0, 0.0}}, {2}).cplxpair();
  } catch (const std::exception& error) {
    show_error(error);
  }

  const auto wave = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  heading("dct_vector");
  show(wave.dct());

  heading("dct_matrix_axes");
  const auto matrix = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  show(matrix.dct());
  show(matrix.dct(0));

  heading("dct_negative_axis");
  std::cout << std::boolalpha << matrix.dct(-1).equal(matrix.dct(1)) << "\n\n";

  heading("dct_round_trip");
  show(wave.dct().idct());

  heading("dct_integer_promotion");
  const auto integer_dct = pluss::make_series({1, 0, -1, 0}).dct();
  show(integer_dct);
  show_meta(integer_dct);

  heading("dct_reverse");
  auto dct_x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  dct_x.tracks_slope_(true);
  dct_x.dct().mul(pluss::make_series({1.0, 0.0, -1.0, 2.0})).sum().reverse();
  show(dct_x.sensitivity());

  heading("dct_axis_error");
  try {
    (void)matrix.dct(2);
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("dst_vector");
  show(wave.dst());

  heading("dst_matrix_axes");
  show(matrix.dst());
  show(matrix.dst(0));

  heading("dst_round_trip");
  show(wave.dst().idst());
  show(matrix.dst().idst());

  heading("dst_reverse");
  auto dst_x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  dst_x.tracks_slope_(true);
  dst_x.dst().mul(pluss::make_series({1.0, 0.0, -1.0, 2.0})).sum().reverse();
  show(dst_x.sensitivity());

  heading("ediff1d_vector");
  show(pluss::make_series({3, 8, 6, 10}).ediff1d());

  heading("ediff1d_flattens");
  show(pluss::make_series({{1, 4, 9}, {16, 25, 36}}).ediff1d());

  heading("ediff1d_complex");
  show(pluss::make_series(std::vector<Complex>{
      {1.0, 1.0}, {4.0, -1.0}, {2.0, 3.0}}, {3}).ediff1d());

  heading("ediff1d_singleton");
  const auto singleton_diff = pluss::make_series({7.0}).ediff1d();
  show(singleton_diff);
  show_meta(singleton_diff);

  heading("ediff1d_reverse");
  auto diff_x = pluss::make_series({2.0, 5.0, 11.0, 20.0});
  diff_x.tracks_slope_(true);
  diff_x.ediff1d().mul(pluss::make_series({1.0, 2.0, 4.0})).sum().reverse();
  show(diff_x.sensitivity());

  heading("elu_default");
  show(pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).elu());

  heading("elu_alpha");
  show(pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).elu(2.0));

  heading("elu_full_parameters");
  show(pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}).elu(2.0, 0.5, 0.25));

  heading("elu_integer_matrix");
  const auto integer_elu = pluss::make_series({{-2, 0}, {1, 3}}).elu();
  show(integer_elu);
  show_meta(integer_elu);

  heading("elu_reverse");
  auto elu_x = pluss::make_series({-2.0, -1.0, 0.0, 1.0});
  elu_x.tracks_slope_(true);
  elu_x.elu(2.0, 0.5, 0.25).sum().reverse();
  show(elu_x.sensitivity());

  heading("elu_zero_scale");
  show(pluss::make_series({-2.0, 0.0, 2.0}).elu(3.0, 0.0, 2.0));

  heading("fftshift_vector_even");
  show(pluss::series::arange(0, 8, 1, pluss::ScalarType::Int64).fftshift());

  heading("fftshift_vector_odd");
  show(pluss::series::arange(0, 7, 1, pluss::ScalarType::Int64).fftshift());

  heading("fftshift_all_axes");
  const auto grid = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64).reshape({3, 4});
  show(grid.fftshift());

  heading("fftshift_selected_axis");
  const std::vector<std::int64_t> rows{0};
  const std::vector<std::int64_t> last{-1};
  show(grid.fftshift(rows));
  show(grid.fftshift(last));

  heading("fftshift_inverse_round_trip");
  std::cout << grid.fftshift().ifftshift().equal(grid) << "\n\n";

  heading("fftshift_reverse");
  auto shift_x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
  shift_x.tracks_slope_(true);
  shift_x.fftshift().mul(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})).sum().reverse();
  show(shift_x.sensitivity());

  heading("fftshift_axis_error");
  try {
    const std::vector<std::int64_t> invalid{2};
    (void)grid.fftshift(invalid);
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("hardsigmoid_boundaries");
  show(pluss::make_series({-4.0, -3.0, -1.5, 0.0, 1.5, 3.0, 4.0}).hardsigmoid());

  heading("hardsigmoid_integer_matrix");
  const auto integer_hsig = pluss::make_series({{-6, -3, 0}, {1, 3, 6}}).hardsigmoid();
  show(integer_hsig);
  show_meta(integer_hsig);

  heading("hardsigmoid_reverse");
  auto hsig_x = pluss::make_series({-4.0, -3.0, -1.0, 0.0, 2.0, 3.0, 4.0});
  hsig_x.tracks_slope_(true);
  hsig_x.hardsigmoid().sum().reverse();
  show(hsig_x.sensitivity());

  heading("hardsigmoid_gate");
  show(pluss::make_series({10.0, 20.0, 30.0}).mul(
      pluss::make_series({-6.0, 0.0, 6.0}).hardsigmoid()));

  heading("hardswish_boundaries");
  show(pluss::make_series({-4.0, -3.0, -1.5, 0.0, 1.5, 3.0, 4.0}).hardswish());

  heading("hardswish_integer_matrix");
  const auto integer_hswish = pluss::make_series({{-4, -2, 0}, {1, 3, 4}}).hardswish();
  show(integer_hswish);
  show_meta(integer_hswish);

  heading("hardswish_reverse");
  auto hswish_x = pluss::make_series({-4.0, -3.0, -1.5, 0.0, 1.5, 3.0, 4.0});
  hswish_x.tracks_slope_(true);
  hswish_x.hardswish().sum().reverse();
  show(hswish_x.sensitivity());

  heading("hardswish_gate");
  show(pluss::make_series({10.0, 20.0, 30.0}).mul(
      pluss::make_series({-4.0, 0.0, 4.0}).hardswish()));

  heading("hardswish_complex_error");
  try {
    (void)pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).hardswish();
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("hilbert_even_signal");
  show(wave.hilbert());

  heading("hilbert_odd_signal");
  show(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}).hilbert());

  heading("hilbert_matrix_axes");
  show(matrix.hilbert());
  show(matrix.hilbert(0));

  heading("hilbert_negative_axis");
  std::cout << matrix.hilbert(-1).allclose(matrix.hilbert(1), 1e-12, 1e-12) << "\n\n";

  heading("hilbert_real_projection");
  show(wave.hilbert().real());

  heading("hilbert_complex_projection");
  show(pluss::make_series(std::vector<Complex>{
      {1.0, 1.0}, {2.0, 0.0}, {3.0, -1.0}, {4.0, 0.0}}, {4}).hilbert());

  heading("hilbert_reverse_real_projection");
  auto hilbert_x = wave.clone();
  hilbert_x.tracks_slope_(true);
  hilbert_x.hilbert().real().mul(pluss::make_series({1.0, 2.0, 3.0, 4.0})).sum().reverse();
  show(hilbert_x.sensitivity());

  heading("hilbert_axis_error");
  try {
    (void)matrix.hilbert(2);
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("idct_coefficients");
  show(pluss::make_series({10.0, -3.154322, 0.0, -0.224171}).idct());

  heading("idct_matrix_axes");
  show(matrix.idct());
  show(matrix.idct(0));

  heading("idct_round_trip");
  show(wave.dct().idct());

  heading("idct_integer_promotion");
  const auto integer_idct = pluss::make_series({4, 2, 0, -1}).idct();
  show(integer_idct);
  show_meta(integer_idct);

  heading("idct_reverse");
  auto idct_x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  idct_x.tracks_slope_(true);
  idct_x.idct().mul(pluss::make_series({1.0, 0.0, -1.0, 2.0})).sum().reverse();
  show(idct_x.sensitivity());

  heading("idct_failures");
  try {
    (void)matrix.idct(2);
  } catch (const std::exception& error) {
    show_error(error);
  }
  try {
    (void)pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).idct();
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("idst_coefficients");
  show(pluss::make_series({6.532815, -2.828427, 2.705981, -2.0}).idst());

  heading("idst_matrix_axes");
  show(matrix.idst());
  show(matrix.idst(0));

  heading("idst_round_trip_even_odd");
  show(wave.dst().idst());
  show(matrix.dst().idst());

  heading("idst_integer_promotion");
  const auto integer_idst = pluss::make_series({4, 2, 0, -1}).idst();
  show(integer_idst);
  show_meta(integer_idst);

  heading("idst_reverse");
  auto idst_x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  idst_x.tracks_slope_(true);
  idst_x.idst().mul(pluss::make_series({1.0, 0.0, -1.0, 2.0})).sum().reverse();
  show(idst_x.sensitivity());

  heading("idst_failures");
  try {
    (void)matrix.idst(-3);
  } catch (const std::exception& error) {
    show_error(error);
  }
  try {
    (void)pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).idst();
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("ifftshift_vector_even");
  show(pluss::series::arange(0, 8, 1, pluss::ScalarType::Int64).ifftshift());

  heading("ifftshift_vector_odd");
  show(pluss::series::arange(0, 7, 1, pluss::ScalarType::Int64).ifftshift());

  heading("ifftshift_all_axes");
  show(grid.ifftshift());

  heading("ifftshift_selected_axis");
  show(grid.ifftshift(rows));
  show(grid.ifftshift(last));

  heading("ifftshift_round_trip");
  std::cout << grid.ifftshift().fftshift().equal(grid) << "\n\n";

  heading("ifftshift_repeated_axis");
  const std::vector<std::int64_t> twice{1, 1};
  show(grid.ifftshift(twice));

  heading("ifftshift_reverse");
  auto inverse_shift_x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
  inverse_shift_x.tracks_slope_(true);
  inverse_shift_x.ifftshift()
      .mul(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}))
      .sum()
      .reverse();
  show(inverse_shift_x.sensitivity());

  heading("ifftshift_axis_error");
  try {
    const std::vector<std::int64_t> invalid_inverse{2};
    (void)grid.ifftshift(invalid_inverse);
  } catch (const std::exception& error) {
    show_error(error);
  }

  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const auto special = pluss::make_series({-inf, -1.0, -0.0, 0.0, 1.0, inf, nan});
  heading("directional_inf_mixed");
  show(special.isneginf());
  show(special.isposinf());

  heading("directional_inf_matrix");
  show(pluss::make_series({{-inf, 2.0}, {inf, nan}}).isneginf());
  show(pluss::make_series({{-inf, 2.0}, {inf, nan}}).isposinf());

  heading("directional_inf_integer");
  show(pluss::make_series({-2, 0, 2}).isneginf());
  show(pluss::make_series({-2, 0, 2}).isposinf());

  heading("directional_inf_partition");
  show(special.isneginf().logical_or(special.isposinf()));

  heading("directional_inf_complex_errors");
  try {
    (void)pluss::make_series(std::vector<Complex>{{inf, 0.0}}, {1}).isneginf();
  } catch (const std::exception& error) {
    show_error(error);
  }
  try {
    (void)pluss::make_series(std::vector<Complex>{{inf, 0.0}}, {1}).isposinf();
  } catch (const std::exception& error) {
    show_error(error);
  }
}
