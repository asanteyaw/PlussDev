#include <pluss/pluss.h>

#include <complex>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

using Complex = std::complex<double>;

void heading(const char* label) { std::cout << "=== " << label << " ===\n"; }

void show(const pluss::Series& value) {
  pluss::series::print(std::cout, value) << "\n\n";
}

void show_error(const std::exception& error) {
  const std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n\n";
}

}  // namespace

int main() {
  const auto signal = pluss::make_series(
      {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});

  heading("stft_default");
  show(signal.stft(4));

  heading("stft_uncentered");
  show(signal.stft(4, 2, 4, pluss::series::undefined(), false,
                   "constant", false, true, true));

  heading("stft_windowed_normalized");
  const auto window = pluss::make_series({0.5, 1.0, 1.0, 0.5});
  show(signal.stft(4, 2, 4, window, true, "reflect", true, true, true));

  heading("stft_two_sided");
  show(signal.stft(4, 2, 4, pluss::series::undefined(), true,
                   "reflect", false, false, true));

  heading("stft_packed");
  const auto packed = signal.stft(4, 2, 4, pluss::series::undefined(), true,
                                  "reflect", false, true, false);
  show(packed);

  heading("stft_packed_shape");
  std::cout << "shape=" << packed.sizes() << "\n\n";

  heading("stft_batch_shape");
  const auto batch = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                                         {4.0, 3.0, 2.0, 1.0}});
  const auto batch_spec = batch.stft(4, 2, 4, pluss::series::undefined(),
                                     false, "constant", false, true, true);
  std::cout << "shape=" << batch_spec.sizes() << "\n\n";

  heading("stft_complex");
  const auto complex_signal = pluss::make_series(
      std::vector<Complex>{{1.0, 1.0}, {2.0, 0.0}, {3.0, -1.0}, {4.0, 0.0}}, {4});
  show(complex_signal.stft(4, 4, 4, pluss::series::undefined(), false,
                           "constant", false, false, true));

  heading("stft_reverse");
  auto tracked_signal = signal.clone();
  tracked_signal.tracks_slope_(true);
  tracked_signal.stft(4, 2, 4, pluss::series::undefined(), true,
                      "reflect", false, true, true).real().sum().reverse();
  show(tracked_signal.sensitivity());

  heading("stft_dtype_error");
  try { (void)pluss::make_series({1, 2, 3, 4}).stft(4); }
  catch (const std::exception& error) { show_error(error); }

  heading("stft_complex_onesided_error");
  try {
    (void)complex_signal.stft(4, 4, 4, pluss::series::undefined(), false,
                              "constant", false, true, true);
  } catch (const std::exception& error) { show_error(error); }

  const auto cube = pluss::series::arange(
      1.0, 25.0, 1.0, pluss::ScalarType::Float64).reshape({2, 3, 4});
  heading("sum_to_size_axes");
  show(cube.sum_to_size({1, 3, 1}));

  heading("sum_to_size_scalar");
  show(cube.sum_to_size({}));

  heading("sum_to_size_noop");
  show(cube.sum_to_size({2, 3, 4}));

  heading("sum_to_size_reverse");
  auto tracked_cube = cube.clone();
  tracked_cube.tracks_slope_(true);
  tracked_cube.sum_to_size({1, 3, 1}).sum().reverse();
  show(tracked_cube.sensitivity());

  heading("sum_to_size_error");
  try { (void)cube.sum_to_size({2, 2, 4}); }
  catch (const std::exception& error) { show_error(error); }

  const auto values = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
  heading("threshold_values");
  show(values.threshold(0.5, -9.0));

  heading("threshold_integer_fraction");
  show(pluss::make_series({-1, 0, 1}).threshold(-0.5, 9.5));

  heading("threshold_matrix");
  show(pluss::make_series({{-2.0, 0.0}, {2.0, 4.0}}).threshold(1.0, 10.0));

  heading("threshold_reverse");
  auto tracked_values = values.clone();
  tracked_values.tracks_slope_(true);
  tracked_values.threshold(0.5, -9.0).sum().reverse();
  show(tracked_values.sensitivity());

  heading("threshold_dtype_error");
  try { (void)pluss::make_series({true, false}).threshold(0.0, 1.0); }
  catch (const std::exception& error) { show_error(error); }

  heading("threshold_inplace");
  auto mutable_values = values.clone();
  mutable_values.threshold_(0.0, 7.0);
  show(mutable_values);

  heading("threshold_inplace_identity");
  auto identity = values.clone();
  auto* before = &identity;
  auto& returned = identity.threshold_(0.0, 7.0);
  std::cout << std::boolalpha << "same object=" << (&returned == before) << "\n\n";

  heading("threshold_inplace_matrix");
  auto mutable_matrix =
      pluss::make_series({{-2.0, 0.0}, {2.0, 4.0}});
  mutable_matrix.threshold_(1.0, 10.0);
  show(mutable_matrix);

  heading("unflatten_exact");
  const auto flat = pluss::series::arange(
      0.0, 24.0, 1.0, pluss::ScalarType::Float64).reshape({2, 12});
  show(flat.unflatten(1, {3, 4}));

  heading("unflatten_inferred");
  const auto inferred = flat.unflatten(-1, {2, -1});
  std::cout << "shape=" << inferred.sizes() << "\n\n";

  heading("unflatten_empty");
  const auto empty = pluss::series::empty({0}, pluss::ScalarType::Float64)
                         .unflatten(0, {2, -1});
  std::cout << "shape=" << empty.sizes() << "\n\n";

  heading("unflatten_reverse");
  auto tracked_flat = flat.clone();
  tracked_flat.tracks_slope_(true);
  tracked_flat.unflatten(1, {3, 4}).sum().reverse();
  show(tracked_flat.sensitivity());

  heading("unflatten_error");
  try { (void)flat.unflatten(1, {5, -1}); }
  catch (const std::exception& error) { show_error(error); }

  const auto phase = pluss::make_series({0.0, 1.0, 2.0, 4.0, -2.0, 0.0});
  heading("unwrap_default");
  show(phase.unwrap());

  heading("unwrap_small_discont");
  show(phase.unwrap(0.5));

  heading("unwrap_large_discont");
  show(phase.unwrap(7.0));

  heading("unwrap_axis");
  show(pluss::make_series({{0.0, 4.0}, {0.0, -4.0}}).unwrap(3.141592653589793, 0));

  heading("unwrap_multi_period");
  show(pluss::make_series({0.0, 13.0, -13.0}).unwrap());

  heading("unwrap_integer_promotion");
  show(pluss::make_series({0, 4, -2}).unwrap());

  heading("unwrap_reverse");
  auto tracked_phase = phase.clone();
  tracked_phase.tracks_slope_(true);
  tracked_phase.unwrap().sum().reverse();
  show(tracked_phase.sensitivity());

  heading("unwrap_empty");
  show(pluss::series::empty({0}, pluss::ScalarType::Float64).unwrap());

  heading("unwrap_rank_error");
  try { (void)pluss::make_series(1.0).unwrap(); }
  catch (const std::exception& error) { show_error(error); }
}
