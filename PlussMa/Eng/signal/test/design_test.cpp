#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng/lti.h"
#include "pluss/ma/eng/signal.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace eng_lti = pluss::subsp::eng::lti;
namespace eng_signal = pluss::subsp::eng::signal;

namespace {

using Complex = std::complex<double>;

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void expect_complex_close(const pluss::Series &actual,
                          const std::vector<Complex> &expected,
                          double tolerance = 2e-10) {
  const auto values =
      actual.to(pluss::ScalarType::Complex128).to_vec<Complex>();
  expect(values.size() == expected.size(), "complex result size mismatch");
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    const double scale =
        std::max({1.0, std::abs(values[lane]), std::abs(expected[lane])});
    expect(std::abs(values[lane] - expected[lane]) <= tolerance * scale,
           "complex mismatch at lane " + std::to_string(lane));
  }
}

pluss::Series physical_omega(double sample_rate) {
  const double pi = std::acos(-1.0);
  return pluss::series::make({0.0, pi * sample_rate / 8.0,
                              pi * sample_rate / 4.0, pi * sample_rate / 2.0,
                              pi * sample_rate});
}

void expect_response(const eng_lti::PoleZero &system,
                     const std::vector<Complex> &expected) {
  expect_complex_close(eng_lti::freqresp(system, physical_omega(1000.0)),
                       expected);
  const auto sections = eng_lti::to_sos(system);
  expect(sections.dim() == 2 && sections.size(1) == 6,
         "SOS conversion returned the wrong shape");
  const auto digital = physical_omega(1000.0).div(1000.0);
  auto response =
      pluss::series::ones({digital.numel()}, pluss::ScalarType::Complex128);
  for (std::int64_t row = 0; row < sections.size(0); ++row) {
    const auto section = sections.select(0, row);
    response = response.mul(eng_signal::freqz(section.slice(0, 0, 3),
                                              section.slice(0, 3, 6), digital));
  }
  expect_complex_close(response, expected, 2e-9);
}

} // namespace

int main() {
  const eng_signal::IirOptions low{.band = eng_signal::Band::Low,
                                   .sample_rate = 1000.0};
  const eng_signal::IirOptions high{.band = eng_signal::Band::High,
                                    .sample_rate = 1000.0};
  const eng_signal::IirOptions pass{.band = eng_signal::Band::Pass,
                                    .sample_rate = 1000.0};
  const eng_signal::IirOptions stop{.band = eng_signal::Band::Stop,
                                    .sample_rate = 1000.0};

  const auto butter_low = eng_signal::butter(4, 100.0, low);
  expect(butter_low.zeros().numel() == 4 && butter_low.poles().numel() == 4 &&
             butter_low.is_disc() && butter_low.is_stable(),
         "Butterworth lowpass metadata is wrong");
  expect_response(butter_low, {{1.0, 0.0},
                               {-0.136416376224650, -0.980838472504803},
                               {-0.239164542543391, 0.261116001709636},
                               {0.00725152496830475, 0.00846314104546392},
                               {0.0, 0.0}});

  expect_response(eng_signal::butter(3, 300.0, high),
                  {{0.0, 0.0},
                   {-0.00086329308786581, -0.00289223648695242},
                   {-0.0156503230621326, -0.0223021247157647},
                   {-0.357599379482528, 0.0186321063487065},
                   {1.0, 0.0}});

  expect_response(eng_signal::butter(3, {100.0, 200.0}, pass),
                  {{0.0, 0.0},
                   {-0.0498961251749309, -0.0447808574268156},
                   {0.697026962364524, 0.714681328163253},
                   {-0.128949138153426, 0.063640204221579},
                   {0.0, 0.0}});

  expect_response(eng_signal::butter(3, {100.0, 200.0}, stop),
                  {{1.0, 0.0},
                   {0.666425764400127, -0.742550841386873},
                   {-0.0416451582524281, 0.0406164216273626},
                   {0.437966132528374, 0.887416312073577},
                   {1.0, 0.0}});

  expect_response(eng_signal::cheby1(4, 1.0, 100.0, low),
                  {{0.891250938133758, 0.0},
                   {-0.437684621898886, -0.801954695696163},
                   {0.0998906458924683, 0.185261748034587},
                   {0.00289856931153181, 0.000969331925576017},
                   {0.0, 0.0}});

  expect_response(eng_signal::cheby2(4, 40.0, 100.0, low),
                  {{1.0, 0.0},
                   {-0.280584389980787, 0.198686631842554},
                   {-0.00418148344686209, -0.00789744435224596},
                   {0.0022122244465902, 0.00104340022648012},
                   {0.01, 0.0}});

  const auto analog =
      eng_signal::butter(3, 2.0,
                         eng_signal::IirOptions{.band = eng_signal::Band::Low,
                                                .sample_rate = std::nullopt});
  expect(analog.is_cont() && analog.zeros().numel() == 0 &&
             analog.poles().numel() == 3 && analog.is_stable(),
         "analog Butterworth metadata is wrong");
  expect_complex_close(
      analog.poles(),
      {{-1.0, 1.7320508075688772}, {-2.0, 0.0}, {-1.0, -1.7320508075688772}});
  expect(std::abs(analog.gain().real().item<double>() - 8.0) < 1e-12,
         "analog Butterworth gain is wrong");

  const auto f32 = eng_signal::butter(
      2, 0.2, eng_signal::IirOptions{.dtype = pluss::ScalarType::Float32});
  expect(f32.poles().scalar_type() == pluss::ScalarType::Complex64 &&
             eng_lti::to_sos(f32).scalar_type() == pluss::ScalarType::Float32,
         "design dtype policy is wrong");

  const auto complex_system = eng_lti::zpk(
      pluss::series::make(std::vector<Complex>{{-0.4, 0.1}, {-0.2, -0.3}}),
      pluss::series::make(std::vector<Complex>{{0.2, 0.1}, {0.3, 0.2}}),
      pluss::series::make(std::vector<Complex>{{0.7, -0.2}}).reshape({}), 1.0);
  const auto complex_sos = eng_lti::to_sos(complex_system);
  expect(complex_sos.scalar_type() == pluss::ScalarType::Complex128 &&
             complex_sos.sizes()[0] == 1 && complex_sos.sizes()[1] == 6,
         "complex SOS policy is wrong");
  const auto probe = pluss::series::make({0.1, 0.7, 2.0});
  expect_complex_close(
      eng_signal::freqz(complex_sos.select(0, 0).slice(0, 0, 3),
                        complex_sos.select(0, 0).slice(0, 3, 6), probe),
      eng_lti::freqresp(complex_system, probe).to_vec<Complex>());

  const auto unstable = eng_lti::zpk(
      pluss::series::make(std::vector<Complex>{{-1.0, 0.0}}),
      pluss::series::make(std::vector<Complex>{{1.1, 0.0}}),
      pluss::series::make(std::vector<Complex>{{1.0, 0.0}}).reshape({}), 1.0);
  expect(!unstable.is_stable(), "unstable digital pole was classified stable");

  bool bad_band = false;
  try {
    (void)eng_signal::butter(3, 100.0, pass);
  } catch (const pluss::Error &) {
    bad_band = true;
  }
  expect(bad_band, "scalar cutoff accepted a band-pass request");

  bool bad_nyquist = false;
  try {
    (void)eng_signal::butter(3, 500.0, low);
  } catch (const pluss::Error &) {
    bad_nyquist = true;
  }
  expect(bad_nyquist, "Nyquist cutoff was accepted");

  bool bad_ripple = false;
  try {
    (void)eng_signal::cheby1(3, 0.0, 100.0, low);
  } catch (const pluss::Error &) {
    bad_ripple = true;
  }
  expect(bad_ripple, "zero Chebyshev ripple was accepted");

  bool analog_sos = false;
  try {
    (void)eng_lti::to_sos(analog);
  } catch (const pluss::Error &) {
    analog_sos = true;
  }
  expect(analog_sos, "analog poles were accepted by delay-form SOS");

  bool improper = false;
  try {
    (void)eng_lti::zpk(
        pluss::series::make(std::vector<Complex>{{0.0, 0.0}, {-1.0, 0.0}}),
        pluss::series::make(std::vector<Complex>{{0.5, 0.0}}),
        pluss::series::make(std::vector<Complex>{{1.0, 0.0}}).reshape({}), 1.0);
  } catch (const pluss::Error &) {
    improper = true;
  }
  expect(improper, "improper pole-zero model was accepted");

  bool nonfinite = false;
  try {
    (void)eng_lti::zpk(
        pluss::series::make(std::vector<Complex>{
            {std::numeric_limits<double>::infinity(), 0.0}}),
        pluss::series::make(std::vector<Complex>{{0.5, 0.0}}),
        pluss::series::make(std::vector<Complex>{{1.0, 0.0}}).reshape({}), 1.0);
  } catch (const pluss::Error &) {
    nonfinite = true;
  }
  expect(nonfinite, "non-finite root was accepted");

  return 0;
}
