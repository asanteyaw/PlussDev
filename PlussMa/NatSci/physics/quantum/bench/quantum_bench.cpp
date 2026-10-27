#include "pluss/ma/natsci/physics.h"
#include "pluss/subsp/linearalg/linearalg.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <vector>

namespace quantum = pluss::subsp::natsci::physics::quantum;

namespace {

template <class Fn> double median_ms(Fn &&fn, int rounds = 11) {
  std::vector<double> elapsed;
  elapsed.reserve(rounds);
  for (int round = 0; round < rounds; ++round) {
    const auto start = std::chrono::steady_clock::now();
    fn();
    const auto stop = std::chrono::steady_clock::now();
    elapsed.push_back(
        std::chrono::duration<double, std::milli>(stop - start).count());
  }
  std::sort(elapsed.begin(), elapsed.end());
  return elapsed[elapsed.size() / 2];
}

pluss::Series unite_pages(std::vector<pluss::Series> &pages) {
  return pages.front().unite(pluss::msharp::RangeView::Of<pluss::Series>(
                                 pages.data() + 1, pages.size() - 1),
                             0);
}

} // namespace

int main() {
  constexpr std::int64_t states = 32768;
  constexpr std::int64_t width = 8;
  const pluss::Series raw =
      pluss::series::arange(0, states * width, 1, pluss::ScalarType::Float64)
          .reshape({16, 16, 8, 16, width})
          .mul(0.0001)
          .sin()
          .add(1.25);
  const auto batch = quantum::normalize(quantum::ket(raw));
  const auto observable = quantum::op(
      pluss::series::arange(1, width + 1, 1, pluss::ScalarType::Float64)
          .diag());
  const pluss::Series expected = quantum::expect(observable, batch);

  pluss::Series batched = pluss::series::undefined();
  const double batched_ms =
      median_ms([&] { batched = quantum::expect(observable, batch); });

  pluss::Series pages = pluss::series::undefined();
  const double page_ms = median_ms([&] {
    std::vector<pluss::Series> parts;
    parts.reserve(16);
    for (std::int64_t index = 0; index < 16; ++index) {
      const auto page =
          quantum::ket(batch.value().select(0, index), batch.spaces());
      parts.push_back(quantum::expect(observable, page).unsqueeze(0));
    }
    pages = unite_pages(parts);
  });

  if (!batched.allclose(expected, 1e-12, 1e-12) ||
      !pages.allclose(expected, 1e-12, 1e-12)) {
    std::cerr << "quantum benchmark paths disagree\n";
    return 1;
  }

  constexpr std::int64_t matrices = 512;
  constexpr std::int64_t order = 4;
  const pluss::Series seed =
      pluss::series::arange(0, matrices * order * order, 1,
                            pluss::ScalarType::Float64)
          .reshape({matrices, order, order})
          .mul(0.013)
          .sin();
  const pluss::Series real = seed.add(seed.adjoint()).mul(0.5);
  const pluss::Series imag = seed.sub(seed.adjoint()).mul(0.125);
  const pluss::Series hermitian = pluss::series::complex(real, imag);
  const pluss::Series spectrum_expected = hermitian.linearalg.eigvalsh();

  pluss::Series spectrum_batch = pluss::series::undefined();
  const double spectrum_batch_ms =
      median_ms([&] { spectrum_batch = hermitian.linearalg.eigvalsh(); }, 7);
  pluss::Series spectrum_pages = pluss::series::undefined();
  const double spectrum_page_ms = median_ms(
      [&] {
        std::vector<pluss::Series> parts;
        parts.reserve(matrices);
        for (std::int64_t index = 0; index < matrices; ++index)
          parts.push_back(
              hermitian.select(0, index).linearalg.eigvalsh().unsqueeze(0));
        spectrum_pages = unite_pages(parts);
      },
      7);
  if (!spectrum_batch.allclose(spectrum_expected, 1e-11, 1e-11) ||
      !spectrum_pages.allclose(spectrum_expected, 1e-11, 1e-11)) {
    std::cerr << "quantum spectral benchmark paths disagree\n";
    return 1;
  }

  const auto diagonal_h = quantum::op(
      pluss::series::arange(0, width, 1, pluss::ScalarType::Float64).diag());
  const auto initial = quantum::ket(pluss::series::complex(
      pluss::series::eye(1, width).reshape({width}),
      pluss::series::zeros({width}, pluss::ScalarType::Float64),
      pluss::ScalarType::Complex128));
  const pluss::Series times =
      pluss::series::linspace(0.0, 2.0 * std::numbers::pi, 256);
  const pluss::Series path_expected =
      quantum::schrod(diagonal_h, initial, times).value();
  pluss::Series path_batch = pluss::series::undefined();
  const double path_batch_ms = median_ms(
      [&] { path_batch = quantum::schrod(diagonal_h, initial, times).value(); },
      7);
  pluss::Series path_pages = pluss::series::undefined();
  const std::vector<double> host_times = times.host<double>();
  const double path_page_ms = median_ms(
      [&] {
        std::vector<pluss::Series> parts;
        parts.reserve(host_times.size());
        for (const double time : host_times) {
          parts.push_back(quantum::unitary(diagonal_h, time)
                              .value()
                              .matmul(initial.value().unsqueeze(-1))
                              .squeeze(-1)
                              .unsqueeze(0));
        }
        path_pages = unite_pages(parts);
      },
      7);
  if (!path_batch.allclose(path_expected, 1e-11, 1e-11) ||
      !path_pages.allclose(path_expected, 1e-11, 1e-11)) {
    std::cerr << "quantum propagation benchmark paths disagree\n";
    return 1;
  }

  std::cout << std::fixed << std::setprecision(3)
            << "shape=[16,16,8,16,8] states=" << states << '\n'
            << "expect_batched_ms=" << batched_ms << '\n'
            << "expect_page_ms=" << page_ms << '\n'
            << "expect_speedup=" << page_ms / batched_ms << "x\n"
            << "expect_checksum=" << batched.sum().item<double>() << '\n'
            << "eigh_matrices=" << matrices << " order=" << order << '\n'
            << "eigh_batched_ms=" << spectrum_batch_ms << '\n'
            << "eigh_page_ms=" << spectrum_page_ms << '\n'
            << "eigh_speedup=" << spectrum_page_ms / spectrum_batch_ms << "x\n"
            << "schrod_times=" << times.numel() << '\n'
            << "schrod_batched_ms=" << path_batch_ms << '\n'
            << "schrod_page_ms=" << path_page_ms << '\n'
            << "schrod_speedup=" << path_page_ms / path_batch_ms << "x\n";
}
