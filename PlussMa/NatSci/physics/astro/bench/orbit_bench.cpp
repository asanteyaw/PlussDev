#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
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
  return astro::make_elem(
      common::SeriesQuantity<common::Length>::from_si(axis),
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
  const auto dim = [index](const common::SeriesQuantity<common::Dimensionless> &value) {
    return common::SeriesQuantity<common::Dimensionless>::from_si(
        value.si_value().select(0, index));
  };
  return astro::make_elem(
      common::SeriesQuantity<common::Length>::from_si(
          elem.axis().si_value().select(0, index)),
      dim(elem.ecc()), dim(elem.tilt()), dim(elem.node()), dim(elem.peri()),
      dim(elem.phase()));
}

pluss::Series unite_pages(std::vector<pluss::Series> &pages) {
  return pages.front().unite(
      pluss::msharp::RangeView::Of<pluss::Series>(pages.data() + 1,
                                                  pages.size() - 1),
      0);
}

} // namespace

int main() {
  const auto elem = elements();
  const auto gm = common::ScalarQuantity<astro::GravParam>::from_si(3.986004418e14);
  const auto expected = astro::from_elem(elem, gm);

  pluss::Series batch_result = pluss::series::undefined();
  const double batch_ms = median_ms([&] {
    batch_result = astro::from_elem(elem, gm).position().si_value();
  });

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

  std::cout << std::fixed << std::setprecision(3)
            << "shape=[16,16,8,16] states=32768\n"
            << "conversion_batched_ms=" << batch_ms << '\n'
            << "conversion_page_ms=" << page_ms << '\n'
            << "conversion_speedup=" << page_ms / batch_ms << "x\n"
            << "propagation_batched_ms=" << advance_batch_ms << '\n'
            << "propagation_page_ms=" << advance_page_ms << '\n'
            << "propagation_speedup=" << advance_page_ms / advance_batch_ms
            << "x\n"
            << "state_checksum=" << batch_result.sum().item<double>() << '\n'
            << "phase_checksum=" << advance_batch.sum().item<double>() << '\n';
}
