#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace astro = pluss::subsp::natsci::physics::astro;

namespace {

astro::Elements elements() {
  const pluss::Series axis =
      pluss::series::arange(0, 32768, 1, pluss::ScalarType::Float64)
          .reshape({16, 16, 8, 16})
          .mul(1e-5)
          .add(7.0e6);
  const pluss::Series phase =
      pluss::series::arange(0, 32768, 1, pluss::ScalarType::Float64)
          .reshape({16, 16, 8, 16})
          .mul(1e-4);
  const auto dim = [](const pluss::Series &value) {
    return common::SeriesQuantity<common::Dimensionless>::from_si(value);
  };
  return astro::make_elem(common::SeriesQuantity<common::Length>::from_si(axis),
                          dim(pluss::series::full_like(axis, 0.2)),
                          dim(pluss::series::full_like(axis, 0.4)),
                          dim(pluss::series::full_like(axis, 0.7)),
                          dim(pluss::series::full_like(axis, 1.1)), dim(phase));
}

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

astro::Elements page(const astro::Elements &elem, std::int64_t index) {
  const auto dim =
      [index](const common::SeriesQuantity<common::Dimensionless> &value) {
        return common::SeriesQuantity<common::Dimensionless>::from_si(
            value.si_value().select(0, index));
      };
  return astro::make_elem(common::SeriesQuantity<common::Length>::from_si(
                              elem.axis().si_value().select(0, index)),
                          dim(elem.ecc()), dim(elem.tilt()), dim(elem.node()),
                          dim(elem.peri()), dim(elem.phase()));
}

astro::OrbitState page(const astro::OrbitState &state, std::int64_t index) {
  return astro::make_state(common::SeriesQuantity<common::Length>::from_si(
                               state.position().si_value().select(0, index)),
                           common::SeriesQuantity<common::Speed>::from_si(
                               state.velocity().si_value().select(0, index)));
}

pluss::Series unite_pages(std::vector<pluss::Series> &pages) {
  return pages.front().unite(pluss::msharp::RangeView::Of<pluss::Series>(
                                 pages.data() + 1, pages.size() - 1),
                             0);
}

} // namespace

int main() {
  const auto elem = elements();
  const auto gm =
      common::ScalarQuantity<astro::GravParam>::from_si(3.986004418e14);
  const auto expected = astro::from_elem(elem, gm);

  pluss::Series batch_result = pluss::series::undefined();
  const double batch_ms = median_ms(
      [&] { batch_result = astro::from_elem(elem, gm).position().si_value(); });

  pluss::Series page_result = pluss::series::undefined();
  const double page_ms = median_ms([&] {
    std::vector<pluss::Series> pages;
    pages.reserve(16);
    for (std::int64_t page_index = 0; page_index < 16; ++page_index) {
      const auto one = page(elem, page_index);
      pages.push_back(
          astro::from_elem(one, gm).position().si_value().unsqueeze(0));
    }
    page_result = unite_pages(pages);
  });

  if (!batch_result.allclose(expected.position().si_value(), 1e-12, 1e-12) ||
      !page_result.allclose(expected.position().si_value(), 1e-12, 1e-12)) {
    std::cerr << "orbit benchmark paths disagree\n";
    return 1;
  }

  const auto elapsed = common::ScalarQuantity<common::Time>::from_si(600.0);
  const pluss::Series expected_phase =
      astro::advance(elem, elapsed, gm).elements.phase().si_value();
  pluss::Series advance_batch = pluss::series::undefined();
  const double advance_batch_ms = median_ms([&] {
    advance_batch =
        astro::advance(elem, elapsed, gm).elements.phase().si_value();
  });

  pluss::Series advance_pages = pluss::series::undefined();
  const double advance_page_ms = median_ms([&] {
    std::vector<pluss::Series> pages;
    pages.reserve(16);
    for (std::int64_t page_index = 0; page_index < 16; ++page_index) {
      pages.push_back(astro::advance(page(elem, page_index), elapsed, gm)
                          .elements.phase()
                          .si_value()
                          .unsqueeze(0));
    }
    advance_pages = unite_pages(pages);
  });

  if (!advance_batch.allclose(expected_phase, 1e-12, 1e-12) ||
      !advance_pages.allclose(expected_phase, 1e-12, 1e-12)) {
    std::cerr << "propagation benchmark paths disagree\n";
    return 1;
  }

  const pluss::Series expected_state =
      astro::advance(expected, elapsed, gm).state.position().si_value();
  pluss::Series state_batch = pluss::series::undefined();
  const double state_batch_ms = median_ms([&] {
    state_batch =
        astro::advance(expected, elapsed, gm).state.position().si_value();
  });

  pluss::Series state_pages = pluss::series::undefined();
  const double state_page_ms = median_ms([&] {
    std::vector<pluss::Series> pages;
    pages.reserve(16);
    for (std::int64_t page_index = 0; page_index < 16; ++page_index) {
      pages.push_back(astro::advance(page(expected, page_index), elapsed, gm)
                          .state.position()
                          .si_value()
                          .unsqueeze(0));
    }
    state_pages = unite_pages(pages);
  });

  if (!state_batch.allclose(expected_state, 1e-12, 1e-9) ||
      !state_pages.allclose(expected_state, 1e-12, 1e-9)) {
    std::cerr << "state propagation benchmark paths disagree\n";
    return 1;
  }

  const pluss::Series field = pluss::series::zeros(
      {16, 16, 8, 16},
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const pluss::Series one = pluss::series::ones_like(field);
  const auto transfer_start =
      common::SeriesQuantity<common::Length>::from_si(
          one.unsqueeze(-1).unite(
              {field.unsqueeze(-1), field.unsqueeze(-1)}, -1));
  const auto transfer_finish =
      common::SeriesQuantity<common::Length>::from_si(
          field.unsqueeze(-1).unite(
              {one.unsqueeze(-1), field.unsqueeze(-1)}, -1));
  const auto transfer_time = common::ScalarQuantity<common::Time>::from_si(
      std::numbers::pi / 2.0);
  const auto transfer_mu =
      common::ScalarQuantity<astro::GravParam>::from_si(1.0);
  const pluss::Series expected_depart =
      astro::lambert(transfer_start, transfer_finish, transfer_time, transfer_mu)
          .depart.si_value();

  pluss::Series lambert_batch = pluss::series::undefined();
  const double lambert_batch_ms = median_ms(
      [&] {
        lambert_batch =
            astro::lambert(transfer_start, transfer_finish, transfer_time,
                           transfer_mu)
                .depart.si_value();
      },
      5);

  pluss::Series lambert_pages = pluss::series::undefined();
  const double lambert_page_ms = median_ms(
      [&] {
        std::vector<pluss::Series> pages;
        pages.reserve(16);
        for (std::int64_t page_index = 0; page_index < 16; ++page_index) {
          const auto start_page =
              common::SeriesQuantity<common::Length>::from_si(
                  transfer_start.si_value().select(0, page_index));
          const auto finish_page =
              common::SeriesQuantity<common::Length>::from_si(
                  transfer_finish.si_value().select(0, page_index));
          pages.push_back(astro::lambert(start_page, finish_page, transfer_time,
                                         transfer_mu)
                              .depart.si_value()
                              .unsqueeze(0));
        }
        lambert_pages = unite_pages(pages);
      },
      5);

  if (!lambert_batch.allclose(expected_depart, 1e-12, 1e-12) ||
      !lambert_pages.allclose(expected_depart, 1e-12, 1e-12)) {
    std::cerr << "Lambert benchmark paths disagree\n";
    return 1;
  }

  std::cout << std::fixed << std::setprecision(3)
            << "shape=[16,16,8,16] states=32768\n"
            << "conversion_batched_ms=" << batch_ms << '\n'
            << "conversion_page_ms=" << page_ms << '\n'
            << "conversion_speedup=" << page_ms / batch_ms << "x\n"
            << "propagation_batched_ms=" << advance_batch_ms << '\n'
            << "propagation_page_ms=" << advance_page_ms << '\n'
            << "propagation_speedup=" << advance_page_ms / advance_batch_ms
            << "x\n"
            << "state_propagation_batched_ms=" << state_batch_ms << '\n'
            << "state_propagation_page_ms=" << state_page_ms << '\n'
            << "state_propagation_speedup=" << state_page_ms / state_batch_ms
            << "x\n"
            << "lambert_batched_ms=" << lambert_batch_ms << '\n'
            << "lambert_page_ms=" << lambert_page_ms << '\n'
            << "lambert_speedup=" << lambert_page_ms / lambert_batch_ms
            << "x\n"
            << "state_checksum=" << batch_result.sum().item<double>() << '\n'
            << "phase_checksum=" << advance_batch.sum().item<double>() << '\n'
            << "propagated_state_checksum=" << state_batch.sum().item<double>()
            << '\n'
            << "lambert_checksum=" << lambert_batch.sum().item<double>()
            << '\n';
}
