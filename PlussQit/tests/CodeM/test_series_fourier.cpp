#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <pluss/pluss.h>

#include <cmath>
#include <complex>
#include <cstdint>
#include <numbers>
#include <string>
#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;

std::vector<std::complex<double>>
dft(const std::vector<std::complex<double>> &input, bool inverse) {
  const auto n = static_cast<std::int64_t>(input.size());
  std::vector<std::complex<double>> output(static_cast<std::size_t>(n));
  const double sign = inverse ? 1.0 : -1.0;
  for (std::int64_t k = 0; k < n; ++k) {
    for (std::int64_t lane = 0; lane < n; ++lane) {
      const double angle = sign * 2.0 * std::numbers::pi *
                           static_cast<double>(k * lane) /
                           static_cast<double>(n);
      output[static_cast<std::size_t>(k)] +=
          input[static_cast<std::size_t>(lane)] *
          std::complex<double>(std::cos(angle), std::sin(angle));
    }
    if (inverse)
      output[static_cast<std::size_t>(k)] /= static_cast<double>(n);
  }
  return output;
}

void require_complex_close(const Series &actual,
                           const std::vector<std::complex<double>> &expected,
                           double margin = 1e-10) {
  const auto values =
      actual.to(ScalarType::Complex128).to_vec<std::complex<double>>();
  REQUIRE(values.size() == expected.size());
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    REQUIRE(values[lane].real() ==
            Approx(expected[lane].real()).margin(margin));
    REQUIRE(values[lane].imag() ==
            Approx(expected[lane].imag()).margin(margin));
  }
}

void require_real_close(const Series &actual,
                        const std::vector<double> &expected,
                        double margin = 1e-10) {
  const auto values = actual.to(ScalarType::Float64).to_vec<double>();
  REQUIRE(values.size() == expected.size());
  for (std::size_t lane = 0; lane < values.size(); ++lane) {
    REQUIRE(values[lane] == Approx(expected[lane]).margin(margin));
  }
}

} // namespace

TEST_CASE(
    "Series Fourier transforms match the DFT at radix and Bluestein lengths",
    "[series][fourier]") {
  for (const std::vector<std::complex<double>> values :
       {std::vector<std::complex<double>>{{0, 0}, {1, -0.5}, {2, 0.25}, {3, 1}},
        std::vector<std::complex<double>>{
            {1, 0}, {-2, 1}, {0.5, -1}, {3, 2}, {-0.25, 0.5}}}) {
    const Series input =
        pluss::make_series(values, {static_cast<std::int64_t>(values.size())});
    const Series spectrum = input.fft();
    require_complex_close(spectrum, dft(values, false));
    require_complex_close(spectrum.ifft(), values);
  }
}

TEST_CASE("Series Fourier transforms honor axes, batches, and strided inputs",
          "[series][fourier][batch]") {
  const Series rows = pluss::make_series(
      std::vector<double>{1, 2, 3, 4, 5, -1, 0, 1, 0, -1}, {2, 5});
  const Series by_row = rows.fft(-1);
  REQUIRE(by_row.sizes()[0] == 2);
  REQUIRE(by_row.sizes()[1] == 5);

  std::vector<std::complex<double>> expected;
  for (const std::vector<std::complex<double>> row :
       {std::vector<std::complex<double>>{
            {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}},
        std::vector<std::complex<double>>{
            {-1, 0}, {0, 0}, {1, 0}, {0, 0}, {-1, 0}}}) {
    const auto transformed = dft(row, false);
    expected.insert(expected.end(), transformed.begin(), transformed.end());
  }
  require_complex_close(by_row, expected);

  const Series strided = rows.transpose(0, 1);
  REQUIRE_FALSE(strided.is_contiguous());
  require_complex_close(strided.fft(-1, 0).transpose(0, 1), expected);
  require_real_close(by_row.ifft().real(), rows.to_vec<double>());
}

TEST_CASE("Series Fourier length and normalization contracts are explicit",
          "[series][fourier][norm]") {
  const Series input = pluss::make_series(std::vector<double>{1, 2, 3, 4}, {4});

  require_real_close(input.fft(2).ifft().real(), {1, 2});
  require_real_close(input.fft(6).ifft().real(), {1, 2, 3, 4, 0, 0});

  for (const std::string norm : {"backward", "forward", "ortho"}) {
    require_real_close(input.fft(-1, -1, norm).ifft(-1, -1, norm).real(),
                       {1, 2, 3, 4});
  }

  require_complex_close(input.fft(-1, -1, "forward"),
                        {{2.5, 0}, {-0.5, 0.5}, {-0.5, 0}, {-0.5, -0.5}});
  const double root = std::sqrt(4.0);
  const auto backward = input.fft().to_vec<std::complex<double>>();
  std::vector<std::complex<double>> ortho(backward.size());
  for (std::size_t lane = 0; lane < backward.size(); ++lane) {
    ortho[lane] = backward[lane] / root;
  }
  require_complex_close(input.fft(-1, -1, "ortho"), ortho);
}

TEST_CASE("real Fourier transforms preserve one-sided information",
          "[series][fourier][real]") {
  const Series even =
      pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6});
  const Series even_spectrum = even.rfft();
  REQUIRE(even_spectrum.sizes()[0] == 4);
  require_real_close(even_spectrum.irfft().to(ScalarType::Float64),
                     {1, 2, 3, 4, 5, 6});

  const Series odd =
      pluss::make_series(std::vector<double>{2, -1, 0.5, 4, -3}, {5});
  const Series odd_spectrum = odd.rfft();
  REQUIRE(odd_spectrum.sizes()[0] == 3);
  require_real_close(odd_spectrum.irfft(5), {2, -1, 0.5, 4, -3});

  REQUIRE(pluss::make_series(std::vector<float>{1, 2, 3}, {3})
              .rfft()
              .scalar_type() == ScalarType::Complex64);
  REQUIRE(pluss::make_series(std::vector<double>{1, 2, 3}, {3})
              .rfft()
              .scalar_type() == ScalarType::Complex128);
}

TEST_CASE("Fourier reverse mode uses the transform adjoint",
          "[series][fourier][autodiff]") {
  Series input = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5}, {5})
                     .to(ScalarType::Float64);
  input.tracks_slope_(true);

  input.fft().real().sum().reverse();
  require_real_close(input.sensitivity(), {5, 0, 0, 0, 0}, 1e-9);

  Series round_trip =
      pluss::make_series(std::vector<double>{-2, 0.5, 3, 1}, {4})
          .to(ScalarType::Float64);
  round_trip.tracks_slope_(true);
  round_trip.fft().ifft().real().sum().reverse();
  require_real_close(round_trip.sensitivity(), {1, 1, 1, 1}, 1e-9);
}

TEST_CASE("Fourier transforms reject ambiguous and invalid contracts",
          "[series][fourier][errors]") {
  const Series values =
      pluss::make_series(std::vector<double>{1, 2, 3, 4}, {4});
  const Series complex_values = values.fft();

  REQUIRE_THROWS_WITH(
      pluss::make_series(1.0).fft(),
      Catch::Matchers::ContainsSubstring("at least one dimension"));
  REQUIRE_THROWS_WITH(values.fft(0),
                      Catch::Matchers::ContainsSubstring("greater than zero"));
  REQUIRE_THROWS_WITH(values.fft(-1, 2),
                      Catch::Matchers::ContainsSubstring("dim out of range"));
  REQUIRE_THROWS_WITH(values.fft(-1, -1, "unitary"),
                      Catch::Matchers::ContainsSubstring("norm must be"));
  REQUIRE_THROWS_WITH(complex_values.rfft(),
                      Catch::Matchers::ContainsSubstring("real-valued"));
}

TEST_CASE("Fourier transforms reproduce reference values from independent "
          "implementations",
          "[series][fourier][reference]") {
  // Values computed in double precision by independent FFT libraries (they
  // agree with this implementation to rounding).
  const Series real7 = pluss::make_series(
      std::vector<double>{0.5, -1.25, 2.0, 3.5, -0.75, 1.0, 0.25}, {7});
  require_complex_close(real7.rfft(),
                        {{5.25, 0.0},
                         {-3.268716990459329, -1.6461865797294006},
                         {-0.2657887146394261, 5.219059407879421},
                         {2.659505705098755, -2.7107865356283836}},
                        1e-12);

  const Series spectrum5 = pluss::make_series(
      std::vector<std::complex<double>>{
          {1, 2}, {-0.5, 1}, {3, -1}, {0.25, 0.5}, {2, 0}},
      {5});
  require_complex_close(spectrum5.ifft(-1, -1, "forward"),
                        {{5.750000000000001, 2.5},
                         {-1.2351583780126019, 1.9522936446288386},
                         {-1.2235902865790456, -3.048394042105276},
                         {2.805149766891361, 5.121343058980434},
                         {-1.0964011022997142, 3.474757338496004}},
                        1e-12);

  const Series half = pluss::make_series(
      std::vector<std::complex<double>>{{3, 0}, {1, -2}, {0.5, 1}, {2, 3}},
      {4});
  require_real_close(half.irfft(7),
                     {1.4285714285714284, -0.1436070187207715,
                      1.9437737847700678, -0.23127160306514458,
                      0.4974054117018982, -0.758645650021529,
                      0.2637736467640503},
                     1e-12);
}

TEST_CASE("Fourier transforms stay accurate at large and prime lengths in both "
          "precisions",
          "[series][fourier][accuracy]") {
  // The transform of a unit impulse at lane j has the exact entries exp(-2 pi i
  // j k / n): a closed form that needs no second implementation. A twiddle
  // factor built by repeated multiplication, or a chirp angle that is not
  // reduced modulo 2 n, loses digits that grow with the length.
  const auto impulse_error = [](std::int64_t n, std::int64_t lane,
                                ScalarType dtype) {
    std::vector<double> data(static_cast<std::size_t>(n), 0.0);
    data[static_cast<std::size_t>(lane)] = 1.0;
    const Series spectrum = pluss::make_series(data, {n}).to(dtype).fft();
    const auto values =
        spectrum.to(ScalarType::Complex128).to_vec<std::complex<double>>();
    double worst = 0.0;
    for (std::int64_t k = 0; k < n; ++k) {
      // (j k) mod n keeps the angle small, so the expected value itself is
      // exact to rounding.
      const std::int64_t reduced =
          static_cast<std::int64_t>((static_cast<__int128>(lane) * k) % n);
      const double angle = -2.0 * std::numbers::pi *
                           static_cast<double>(reduced) /
                           static_cast<double>(n);
      worst = std::max(worst, std::abs(values[static_cast<std::size_t>(k)] -
                                       std::complex<double>(std::cos(angle),
                                                            std::sin(angle))));
    }
    return worst;
  };
  REQUIRE(impulse_error(1 << 16, 12345, ScalarType::Float64) < 1e-13);
  REQUIRE(impulse_error(1 << 16, 12345, ScalarType::Float32) < 2e-6);
  REQUIRE(impulse_error(100003, 54321, ScalarType::Float64) < 1e-12);
  REQUIRE(impulse_error(100003, 54321, ScalarType::Float32) < 2e-5);
  REQUIRE(impulse_error(1000003, 777777, ScalarType::Float64) < 1e-11);
  REQUIRE(impulse_error(1000003, 777777, ScalarType::Float32) < 1e-4);
}

TEST_CASE("Fourier transforms of real signals are exactly conjugate-symmetric "
          "and exact on simple data",
          "[series][fourier][exact]") {
  // Multiples of pi / 2 give exactly 0 and +-1, so small integer data
  // transforms to exact small integers.
  require_complex_close(
      pluss::make_series(std::vector<double>{1, 1, 0, 0}, {4}).fft(),
      {{2, 0}, {1, -1}, {0, 0}, {1, 1}}, 0.0);
  require_complex_close(
      pluss::make_series(std::vector<double>{1, 2, 3, 4}, {4}).fft(),
      {{10, 0}, {-2, 2}, {-2, 0}, {-2, -2}}, 0.0);

  for (const std::int64_t n : {3, 5, 6, 7, 8, 9, 12, 15, 16, 100}) {
    std::vector<double> data(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i)
      data[static_cast<std::size_t>(i)] =
          std::sin(0.7 * static_cast<double>(i)) + 0.1 * static_cast<double>(i);
    const auto spectrum =
        pluss::make_series(data, {n}).fft().to_vec<std::complex<double>>();
    REQUIRE(spectrum[0].imag() == 0.0);
    for (std::int64_t k = 1; k < n; ++k) {
      const auto mirror = spectrum[static_cast<std::size_t>(n - k)];
      REQUIRE(spectrum[static_cast<std::size_t>(k)].real() == mirror.real());
      REQUIRE(spectrum[static_cast<std::size_t>(k)].imag() == -mirror.imag());
    }
    if (n % 2 == 0)
      REQUIRE(spectrum[static_cast<std::size_t>(n / 2)].imag() == 0.0);
    const auto one_sided =
        pluss::make_series(data, {n}).rfft().to_vec<std::complex<double>>();
    REQUIRE(one_sided[0].imag() == 0.0);
  }
}

TEST_CASE("Fourier transforms reject unsupported arguments like the reference "
          "library",
          "[series][fft]") {
  const Series x = pluss::make_series(std::vector<double>{1, 2, 3, 4}, {4});
  REQUIRE_THROWS_WITH(x.fft(-1, -1, ""),
                      Catch::Matchers::ContainsSubstring("norm"));
  REQUIRE_THROWS_WITH(x.rfft(-1, -1, ""),
                      Catch::Matchers::ContainsSubstring("norm"));
  REQUIRE_THROWS_WITH(x.to(ScalarType::Float16).fft(),
                      Catch::Matchers::ContainsSubstring("half-precision"));
  REQUIRE_THROWS_WITH(x.to(ScalarType::BFloat16).rfft(),
                      Catch::Matchers::ContainsSubstring("half-precision"));
  REQUIRE_THROWS_WITH(x.to(ScalarType::Float16).irfft(),
                      Catch::Matchers::ContainsSubstring("half-precision"));
}

TEST_CASE(
    "Batched and strided real transforms agree with the direct definition",
    "[series][fft]") {
  // Rows are transformed together as interleaved lanes; lengths, row counts and
  // axes below cover the remainder blocks, the packed real path (even lengths),
  // the odd-length fallback and the non-last axis.
  for (const std::int64_t length : {2, 3, 4, 6, 8, 9, 30, 64}) {
    for (const std::int64_t rows : {1, 3, 5, 17}) {
      std::vector<double> data(static_cast<std::size_t>(rows * length));
      for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = std::sin(0.37 * static_cast<double>(i)) +
                  0.01 * static_cast<double>(i % 7);
      const Series matrix = pluss::make_series(data, {rows, length});
      const auto full = matrix.fft().to_vec<std::complex<double>>();
      const auto half = matrix.rfft().to_vec<std::complex<double>>();
      const std::int64_t m = length / 2 + 1;
      for (std::int64_t r = 0; r < rows; ++r) {
        std::vector<std::complex<double>> row(static_cast<std::size_t>(length));
        for (std::int64_t j = 0; j < length; ++j)
          row[static_cast<std::size_t>(j)] =
              data[static_cast<std::size_t>(r * length + j)];
        const auto expected = dft(row, false);
        for (std::int64_t k = 0; k < length; ++k) {
          REQUIRE(std::abs(full[static_cast<std::size_t>(r * length + k)] -
                           expected[static_cast<std::size_t>(k)]) < 1e-11);
        }
        for (std::int64_t k = 0; k < m; ++k) {
          REQUIRE(half[static_cast<std::size_t>(r * m + k)] ==
                  full[static_cast<std::size_t>(r * length + k)]);
        }
      }
      // The same data transformed along the first axis of the transposed
      // matrix.
      const auto columns = matrix.transpose(0, 1)
                               .contiguous()
                               .fft(-1, 0)
                               .to_vec<std::complex<double>>();
      for (std::int64_t r = 0; r < rows; ++r) {
        for (std::int64_t k = 0; k < length; ++k) {
          REQUIRE(std::abs(columns[static_cast<std::size_t>(k * rows + r)] -
                           full[static_cast<std::size_t>(r * length + k)]) <
                  1e-11);
        }
      }
    }
  }
}

TEST_CASE("Precision promotion between float and complex dtypes keeps values",
          "[series][dtype]") {
  const Series x =
      pluss::make_series(std::vector<double>{1.5, -2.25, 3.0}, {3});
  const Series c64 = x.to(ScalarType::Complex64);
  REQUIRE(c64.scalar_type() == ScalarType::Complex64);
  REQUIRE(c64.to(ScalarType::Complex128).to_vec<std::complex<double>>() ==
          std::vector<std::complex<double>>{{1.5, 0}, {-2.25, 0}, {3.0, 0}});
  const Series z = pluss::make_series(
      std::vector<std::complex<double>>{{1, 2}, {3, -4}}, {2});
  REQUIRE(z.to(ScalarType::Complex64)
              .to(ScalarType::Complex128)
              .to_vec<std::complex<double>>() ==
          std::vector<std::complex<double>>{{1, 2}, {3, -4}});
  REQUIRE(x.to(ScalarType::Float32).to(ScalarType::Float64).to_vec<double>() ==
          std::vector<double>{1.5, -2.25, 3.0});
  REQUIRE(z.to(ScalarType::Float64).to_vec<double>() ==
          std::vector<double>{1, 3});
}
