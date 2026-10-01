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

pluss::Series spectrum(const pluss::Series& signal, bool normalized = false,
                       bool onesided = true) {
  return signal.stft(4, 2, 4, pluss::series::undefined(), true, "reflect",
                     normalized, onesided, true);
}

}  // namespace

int main() {
  heading("isreal_real_matrix");
  show(pluss::make_series({{1.0, -2.0}, {0.0, 4.0}}).isreal());

  heading("isreal_complex_lanes");
  show(pluss::make_series(
      std::vector<Complex>{{1.0, 0.0}, {2.0, -0.0}, {3.0, 4.0}, {0.0, -2.0}}, {2, 2})
           .isreal());

  heading("isreal_scalar");
  show(pluss::make_series(4.5).isreal());

  heading("item_native_types");
  std::cout << std::boolalpha
            << "bool=" << pluss::make_series(true).item<bool>() << '\n'
            << "int=" << pluss::make_series(7).item<std::int64_t>() << '\n'
            << "double=" << pluss::make_series(3.5).item<double>() << "\n\n";

  heading("item_singleton_rank");
  std::cout << pluss::make_series({42.0}).item<double>() << "\n\n";

  heading("item_dynamic");
  std::cout << pluss::make_series(7).item() << '\n'
            << pluss::make_series(2.25).item() << "\n\n";

  heading("item_complex");
  std::cout << pluss::make_series(std::vector<Complex>{{2.0, -3.0}}, {}).item<Complex>()
            << "\n\n";

  heading("item_reduction");
  std::cout << pluss::make_series({1.5, 2.5, 3.5, 4.5}).sum().item<double>()
            << "\n\n";

  heading("item_many_error");
  try {
    (void)pluss::make_series({1, 2}).item<std::int64_t>();
  } catch (const std::exception& error) {
    show_error(error);
  }

  const auto norm_x = pluss::make_series({{1.0, 2.0, 3.0}, {2.0, 4.0, 6.0}});
  const auto norm_w = pluss::make_series({1.0, 1.0, 1.0});
  const auto norm_b = pluss::make_series({0.0, 0.0, 0.0});
  heading("layer_norm_rows");
  show(norm_x.layer_norm({3}, norm_w, norm_b));

  heading("layer_norm_affine");
  show(norm_x.layer_norm({3}, pluss::make_series({1.0, 2.0, 0.5}),
                         pluss::make_series({0.0, 1.0, -1.0})));

  heading("layer_norm_two_axes");
  show(pluss::make_series({{{1.0, 2.0}, {3.0, 4.0}},
                           {{2.0, 4.0}, {6.0, 8.0}}})
           .layer_norm({2, 2}, pluss::series::ones({2, 2}, pluss::ScalarType::Float64),
                       pluss::series::zeros({2, 2}, pluss::ScalarType::Float64)));

  heading("layer_norm_eps");
  show(pluss::make_series({{1.0, 1.0, 1.0}})
           .layer_norm({3}, norm_w, norm_b, 1e-3));

  heading("layer_norm_reverse");
  auto tracked_x = norm_x.clone();
  auto tracked_w = pluss::make_series({1.0, 2.0, 0.5});
  auto tracked_b = pluss::make_series({0.0, 1.0, -1.0});
  tracked_x.tracks_slope_(true);
  tracked_w.tracks_slope_(true);
  tracked_b.tracks_slope_(true);
  tracked_x.layer_norm({3}, tracked_w, tracked_b).sum().reverse();
  show(tracked_x.sensitivity());
  show(tracked_w.sensitivity());
  show(tracked_b.sensitivity());

  heading("layer_norm_shape_error");
  try {
    (void)norm_x.layer_norm({2}, pluss::series::ones({2}, pluss::ScalarType::Float64),
                            pluss::series::zeros({2}, pluss::ScalarType::Float64));
  } catch (const std::exception& error) {
    show_error(error);
  }

  const auto act = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
  heading("mish_values");
  show(act.mish());

  heading("mish_matrix");
  show(pluss::make_series({{-2.0, 0.0}, {1.0, 3.0}}).mish());

  heading("mish_integer_promotion");
  show(pluss::make_series({-2, 0, 2}).mish());

  heading("mish_reverse");
  auto mish_x = pluss::make_series({-1.0, 0.0, 1.0});
  mish_x.tracks_slope_(true);
  mish_x.mish().sum().reverse();
  show(mish_x.sensitivity());

  heading("mish_complex_error");
  try {
    (void)pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).mish();
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("rrelu_default_eval");
  show(act.rrelu());

  heading("rrelu_custom_eval");
  show(act.rrelu(0.2, 0.4, false));

  heading("rrelu_negative_interval");
  show(pluss::make_series({-2.0, -1.0, 0.0, 2.0}).rrelu(-0.4, -0.2, false));

  heading("rrelu_seeded_training");
  pluss::rng_seed(20260928);
  show(act.rrelu(0.2, 0.4, true));

  heading("rrelu_training_reverse");
  pluss::rng_seed(20260928);
  auto rrelu_x = act.clone();
  rrelu_x.tracks_slope_(true);
  rrelu_x.rrelu(0.2, 0.4, true).sum().reverse();
  show(rrelu_x.sensitivity());

  heading("rrelu_bounds_error");
  try {
    (void)act.rrelu(0.5, 0.1, true);
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("rrelu_dtype_error");
  try {
    (void)pluss::make_series({-2, 1}).rrelu();
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("selu_values");
  show(act.selu());

  heading("selu_matrix");
  show(pluss::make_series({{-2.0, 0.0}, {1.0, 3.0}}).selu());

  heading("selu_integer_promotion");
  show(pluss::make_series({-2, 0, 2}).selu());

  heading("selu_reverse");
  auto selu_x = pluss::make_series({-1.0, 0.0, 1.0});
  selu_x.tracks_slope_(true);
  selu_x.selu().sum().reverse();
  show(selu_x.sensitivity());

  heading("selu_complex_error");
  try {
    (void)pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).selu();
  } catch (const std::exception& error) {
    show_error(error);
  }

  const auto signal = pluss::make_series({1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
  heading("istft_round_trip");
  show(spectrum(signal).istft(4, 2, 4, pluss::series::undefined(), true,
                              false, true, 8));

  heading("istft_inferred_defaults");
  show(signal.stft(4).istft(4));

  heading("istft_normalized");
  show(spectrum(signal, true).istft(4, 2, 4, pluss::series::undefined(), true,
                                    true, true, 8));

  heading("istft_explicit_window");
  const auto window = pluss::make_series({0.5, 1.0, 1.0, 0.5});
  show(signal.stft(4, 2, 4, window, true, "reflect", false, true, true)
           .istft(4, 2, 4, window, true, false, true, 8));

  heading("istft_uncentered");
  show(signal.stft(4, 2, 4, pluss::series::undefined(), false, "reflect",
                   false, true, true)
           .istft(4, 2, 4, pluss::series::undefined(), false,
                  false, true, 8));

  heading("istft_full_spectrum");
  show(spectrum(signal, false, false).istft(4, 2, 4, pluss::series::undefined(),
                                            true, false, false, 8));

  heading("istft_batch");
  const auto batch = pluss::make_series({{1.0, 2.0, 3.0, 4.0, 3.0, 2.0, 1.0, 0.0},
                                         {0.0, 1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0}});
  const auto batch_spectrum = spectrum(batch);
  const auto batch_output = batch_spectrum.istft(
      4, 2, 4, pluss::series::undefined(), true, false, true, 8);
  show(batch_output);

  heading("istft_batch_shapes");
  std::cout << "spectrum shape=[" << batch_spectrum.size(0) << ','
            << batch_spectrum.size(1) << ',' << batch_spectrum.size(2)
            << "]\n"
            << "output shape=[" << batch_output.size(0) << ','
            << batch_output.size(1) << "]\n\n";

  heading("istft_requested_shorter");
  show(spectrum(signal).istft(4, 2, 4, pluss::series::undefined(), true,
                              false, true, 5));

  heading("istft_requested_longer");
  show(spectrum(signal).istft(4, 2, 4, pluss::series::undefined(), true,
                              false, true, 11));

  heading("istft_reverse");
  auto spec = spectrum(signal);
  spec.tracks_slope_(true);
  spec.istft(4, 2, 4, pluss::series::undefined(), true, false, true, 8)
      .sum().reverse();
  show(spec.sensitivity().real());

  heading("istft_window_error");
  try {
    (void)spectrum(signal).istft(4, 2, 4, pluss::make_series({1.0, 1.0}));
  } catch (const std::exception& error) {
    show_error(error);
  }

  heading("istft_overlap_error");
  try {
    (void)spectrum(signal).istft(
        4, 2, 4,
        pluss::series::zeros({4}, pluss::ScalarType::Float64));
  } catch (const std::exception& error) {
    show_error(error);
  }
}
