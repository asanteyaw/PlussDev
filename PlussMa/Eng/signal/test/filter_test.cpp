#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng/signal.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <stdexcept>
#include <string>
#include <vector>

namespace eng_signal = pluss::subsp::eng::signal;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void expect_close(const pluss::Series &actual,
                  const std::vector<double> &expected,
                  double tolerance = 1e-10) {
  const auto values = actual.to(pluss::ScalarType::Float64).host<double>();
  expect(values.size() == expected.size(), "signal result size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "signal value mismatch at lane " + std::to_string(lane) + ": got " +
               std::to_string(values[lane]) + ", expected " +
               std::to_string(expected[lane]));
  }
}

void expect_complex_close(const pluss::Series &actual,
                          const std::vector<std::complex<double>> &expected,
                          double tolerance = 1e-10) {
  const auto values =
      actual.to(pluss::ScalarType::Complex128).host<std::complex<double>>();
  expect(values.size() == expected.size(), "complex result size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "complex value mismatch at lane " + std::to_string(lane));
  }
}

} // namespace

int main() {
  using Complex = std::complex<double>;
  const auto x = pluss::series::make({1.0, 2.0, 3.0});
  const auto h = pluss::series::make({1.0, 1.0});
  expect_close(eng_signal::conv(x, h), {1, 3, 5, 3});
  expect_close(eng_signal::conv(x, h, -1, eng_signal::ConvMode::Same),
               {1, 3, 5});
  expect_close(eng_signal::conv(x, h, -1, eng_signal::ConvMode::Valid), {3, 5});
  expect_close(eng_signal::conv(h, x, -1, eng_signal::ConvMode::Valid), {3, 5});
  expect_close(eng_signal::corr(x, pluss::series::make({1.0, 2.0})),
               {2, 5, 8, 3});
  expect_close(eng_signal::fir(x, h), {1, 3, 5});

  const auto iir_b = pluss::series::make({0.5, 0.5});
  const auto iir_a = pluss::series::make({1.0, -0.5});
  const auto stream =
      pluss::series::make({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});
  const auto filtered = eng_signal::filter(stream, iir_b, iir_a);
  expect_close(filtered.values, {0.5, 1.75, 3.375, 5.1875, 7.09375, 9.046875,
                                 11.0234375, 13.01171875});
  expect_close(filtered.state, {10.505859375});
  expect_close(eng_signal::filter(pluss::series::make({1.0, 2.0}),
                                  pluss::series::make({2.0}),
                                  pluss::series::make({2.0, -1.0}))
                   .values,
               {1.0, 2.5});

  const auto empty_run = eng_signal::filter(
      pluss::series::zeros({2, 0}, pluss::ScalarType::Float64), iir_b, iir_a,
      1);
  expect(empty_run.values.sizes()[0] == 2 && empty_run.values.sizes()[1] == 0 &&
             empty_run.state.sizes()[0] == 2 &&
             empty_run.state.sizes()[1] == 1 &&
             empty_run.state.eq(0.0).all().item<bool>(),
         "empty filtering did not preserve lanes and rest state");

  const auto first = eng_signal::filter(stream.slice(0, 0, 4), iir_b, iir_a);
  const auto second =
      eng_signal::filter(stream.slice(0, 4, 8), iir_b, iir_a, first.state);
  expect_close(first.values.unite({second.values}, 0),
               filtered.values.host<double>());
  expect_close(second.state, filtered.state.host<double>());

  const auto lane_input =
      pluss::series::make({{1.0, 2.0, 3.0, 4.0}, {2.0, 4.0, 6.0, 8.0}});
  const auto lane_filter = eng_signal::filter(lane_input, iir_b, iir_a,
                                              pluss::series::make({1.0}), 1);
  expect(lane_filter.values.sizes()[0] == 2 &&
             lane_filter.values.sizes()[1] == 4 &&
             lane_filter.state.sizes()[0] == 2 &&
             lane_filter.state.sizes()[1] == 1,
         "batched filter shape is wrong");
  expect_close(lane_filter.values,
               {1.5, 2.25, 3.625, 5.3125, 2.0, 4.0, 7.0, 10.5});
  expect_close(lane_filter.state, {4.65625, 9.25});

  const auto sections = pluss::series::make(
      {{0.5, 0.5, 0.0, 1.0, -0.5, 0.0}, {1.0, 0.0, 0.0, 1.0, 0.25, 0.0}});
  const auto section_run = eng_signal::sos(stream, sections);
  expect_close(section_run.values,
               {0.5, 1.625, 2.96875, 4.4453125, 5.982421875, 7.55126953125,
                9.1356201171875, 10.727813720703125});
  expect(section_run.state.sizes()[0] == 2 && section_run.state.sizes()[1] == 2,
         "SOS final state shape is wrong");
  expect_close(section_run.state,
               {10.505859375, 0.0, -2.6819534301757812, 0.0});

  const auto phase = eng_signal::zero_phase(
      stream, iir_b, iir_a,
      eng_signal::PhaseOpts{
          .dim = -1, .edge = 3, .pad = eng_signal::PadMode::Odd});
  expect_close(phase,
               {4.369140982627869, 8.175781965255737, 12.070313930511475,
                16.00000286102295, 19.9296932220459, 23.824230194091797,
                27.630882263183594, 31.252975463867188},
               1e-9);
  const auto even_phase = eng_signal::zero_phase(
      stream, iir_b, iir_a,
      eng_signal::PhaseOpts{
          .dim = 0, .edge = 3, .pad = eng_signal::PadMode::Even});
  expect_close(even_phase,
               {9.58325731754303, 10.72901463508606, 13.23927927017212,
                16.36918354034424, 19.683679580688477, 22.840015411376953,
                25.416358947753906, 26.700881958007812},
               1e-9);
  const auto constant_phase = eng_signal::zero_phase(
      stream, iir_b, iir_a,
      eng_signal::PhaseOpts{
          .dim = 0, .edge = 3, .pad = eng_signal::PadMode::Constant});
  expect_close(constant_phase,
               {6.976199150085449, 9.452398300170898, 12.654796600341797,
                16.184593200683594, 19.806686401367188, 23.332122802734375,
                26.52362060546875, 28.9769287109375},
               1e-9);
  const auto short_identity = pluss::series::make({2.0, 5.0});
  expect_close(eng_signal::zero_phase(short_identity,
                                      pluss::series::make({1.0, 0.0, 0.0}),
                                      pluss::series::make({1.0, 0.0, 0.0})),
               {2.0, 5.0});
  expect_close(eng_signal::zero_phase(
                   short_identity,
                   pluss::series::make({{1.0, 0.0, 0.0, 1.0, 0.0, 0.0},
                                        {1.0, 0.0, 0.0, 1.0, 0.0, 0.0}})),
               {2.0, 5.0});
  expect(eng_signal::zero_phase(
             stream, sections,
             eng_signal::PhaseOpts{.pad = eng_signal::PadMode::None})
                 .sizes() == stream.sizes(),
         "SOS zero-phase filtering changed shape");
  const auto sos_phase = eng_signal::zero_phase(
      pluss::series::make({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}),
      sections);
  expect_close(sos_phase,
               {2.58971658617797, 5.13315998449128, 7.68663863633336,
                10.2425711242904, 12.8000122118293, 15.3573769782291,
                17.9135575112852, 20.4660582974849, 23.0134095905057,
                25.5401834094443},
               1e-9);

  const auto complex_filter = eng_signal::filter(
      pluss::series::make(std::vector<Complex>{{1.0, 1.0}, {2.0, -1.0}}),
      pluss::series::make(std::vector<Complex>{{1.0, 0.0}}),
      pluss::series::make(std::vector<Complex>{{1.0, 0.0}, {-0.5, 0.0}}));
  expect_complex_close(complex_filter.values, {{1.0, 1.0}, {2.5, -0.5}});

  const auto strided =
      pluss::series::make({{1.0, 4.0}, {2.0, 5.0}, {3.0, 6.0}}).transpose(0, 1);
  expect(!strided.is_contiguous(), "strided signal became contiguous");
  const auto batched = eng_signal::conv(strided, h, 1);
  expect(batched.sizes()[0] == 2 && batched.sizes()[1] == 4,
         "batched convolution shape is wrong");
  expect_close(batched, {1, 3, 5, 3, 4, 9, 11, 6});
  const auto strided_filter = eng_signal::filter(strided, iir_b, iir_a, 1);
  expect_close(strided_filter.values, {0.5, 1.75, 3.375, 2.0, 5.5, 8.25});
  expect_close(strided_filter.state, {3.1875, 7.125});

  const auto cube =
      pluss::series::make(std::vector<double>{1, 10, 2, 20, 3, 30, 4, 40, 2, 20,
                                              4, 40, 6, 60, 8, 80},
                          {2, 4, 2});
  const auto cube_filter = eng_signal::filter(cube, iir_b, iir_a, 1);
  expect(cube_filter.values.sizes() == cube.sizes() &&
             cube_filter.state.sizes()[0] == 2 &&
             cube_filter.state.sizes()[1] == 2 &&
             cube_filter.state.sizes()[2] == 1,
         "higher-rank filter shape is wrong");
  expect_close(cube_filter.values,
               {0.5, 5, 1.75, 17.5, 3.375, 33.75, 5.1875, 51.875, 1, 10, 3.5,
                35, 6.75, 67.5, 10.375, 103.75});

  const auto complex_x =
      pluss::series::make(std::vector<Complex>{{1.0, 1.0}, {2.0, 0.0}});
  const auto complex_h =
      pluss::series::make(std::vector<Complex>{{1.0, 0.0}, {0.0, -1.0}});
  expect_complex_close(eng_signal::conv(complex_x, complex_h),
                       {{1, 1}, {3, -1}, {0, -2}});

  const double pi = std::acos(-1.0);
  const auto omega = pluss::series::make({0.0, pi});
  expect_complex_close(eng_signal::freqz(h, omega), {{2, 0}, {0, 0}});
  expect_complex_close(eng_signal::freqz(pluss::series::make({1.0}),
                                         pluss::series::make({1.0, -0.5}),
                                         omega),
                       {{2, 0}, {2.0 / 3.0, 0}});
  const auto bank = pluss::series::make({{1.0, 1.0}, {1.0, -1.0}});
  const auto bank_response = eng_signal::freqz(bank, omega);
  expect(bank_response.sizes()[0] == 2 && bank_response.sizes()[1] == 2,
         "filter-bank response shape is wrong");
  expect_complex_close(bank_response, {{2, 0}, {0, 0}, {0, 0}, {2, 0}});

  auto tracked_x = pluss::series::make({1.0, 2.0, 3.0});
  auto tracked_h = pluss::series::make({2.0, 3.0});
  tracked_x.tracks_slope_(true);
  tracked_h.tracks_slope_(true);
  eng_signal::conv(tracked_x, tracked_h).sum().reverse();
  expect_close(tracked_x.sensitivity(), {5, 5, 5});
  expect_close(tracked_h.sensitivity(), {6, 6});

  auto tracked_taps = pluss::series::make({1.0, 1.0});
  tracked_taps.tracks_slope_(true);
  eng_signal::freqz(tracked_taps, omega).real().sum().reverse();
  expect_close(tracked_taps.sensitivity(), {2, 0}, 1e-9);

  auto diff_x = pluss::series::make({1.0, 2.0, 3.0});
  auto diff_b = pluss::series::make({1.0});
  auto diff_a = pluss::series::make({1.0, -0.5});
  auto diff_state = pluss::series::make({0.0});
  diff_x.tracks_slope_(true);
  diff_b.tracks_slope_(true);
  diff_a.tracks_slope_(true);
  diff_state.tracks_slope_(true);
  eng_signal::filter(diff_x, diff_b, diff_a, diff_state).values.sum().reverse();
  expect_close(diff_x.sensitivity(), {1.75, 1.5, 1.0});
  expect_close(diff_b.sensitivity(), {7.75});
  expect_close(diff_a.sensitivity(), {-9.75, -4.0});
  expect_close(diff_state.sensitivity(), {1.75});

  bool integral = false;
  try {
    (void)eng_signal::conv(pluss::series::make({1, 2, 3}), h);
  } catch (const pluss::Error &) {
    integral = true;
  }
  expect(integral, "integral convolution input was accepted");

  bool integral_filter = false;
  try {
    (void)eng_signal::filter(pluss::series::make({1, 2, 3}), iir_b, iir_a);
  } catch (const pluss::Error &) {
    integral_filter = true;
  }
  expect(integral_filter, "integral filtering input was accepted");

  bool scalar_omega = false;
  try {
    (void)eng_signal::freqz(h, pluss::series::make(0.0));
  } catch (const pluss::Error &) {
    scalar_omega = true;
  }
  expect(scalar_omega, "scalar frequency grid was accepted");

  bool zero_a0 = false;
  try {
    (void)eng_signal::filter(stream, iir_b, pluss::series::make({0.0, 1.0}));
  } catch (const pluss::Error &) {
    zero_a0 = true;
  }
  expect(zero_a0, "zero denominator leading coefficient was accepted");

  bool bad_state = false;
  try {
    (void)eng_signal::filter(lane_input, iir_b, iir_a,
                             pluss::series::zeros({3, 2}), 1);
  } catch (const pluss::Error &) {
    bad_state = true;
  }
  expect(bad_state, "incompatible filter state was accepted");

  bool bad_sections = false;
  try {
    (void)eng_signal::sos(stream, pluss::series::zeros({2, 5}));
  } catch (const pluss::Error &) {
    bad_sections = true;
  }
  expect(bad_sections, "malformed SOS matrix was accepted");

  bool short_phase = false;
  try {
    (void)eng_signal::zero_phase(pluss::series::make({1.0, 2.0, 3.0}), iir_b,
                                 iir_a);
  } catch (const pluss::Error &) {
    short_phase = true;
  }
  expect(short_phase, "short zero-phase signal was accepted");

  bool no_steady_state = false;
  try {
    (void)eng_signal::zero_phase(
        stream, pluss::series::make({1.0}), pluss::series::make({1.0, -1.0}),
        eng_signal::PhaseOpts{.pad = eng_signal::PadMode::None});
  } catch (const pluss::Error &) {
    no_steady_state = true;
  }
  expect(no_steady_state,
         "zero-phase filter accepted a denominator without step steady state");
}
