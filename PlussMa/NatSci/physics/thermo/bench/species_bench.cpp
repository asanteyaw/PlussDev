#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace thermo = pluss::subsp::natsci::physics::thermo;

using pluss::ScalarType;
using pluss::Series;

namespace {

using Clock = std::chrono::steady_clock;

thermo::Nasa7 model(std::int64_t species_count) {
  std::vector<thermo::SpeciesData> data;
  data.reserve(static_cast<std::size_t>(species_count));
  for (std::int64_t index = 0; index < species_count; ++index) {
    thermo::SpeciesData species;
    species.name = "S" + std::to_string(index);
    species.t_min = 200.0;
    species.t_mid = 1000.0;
    species.t_max = 5000.0;
    const double shift = static_cast<double>(index) * 1e-3;
    species.low = {3.5 + shift, 1e-3, -2e-7, 3e-11, -1e-15, -900.0, 2.0};
    species.high = {3.0 + shift, 1.5e-3, -4e-7, 5e-11, -2e-15, -850.0, 4.0};
    data.push_back(std::move(species));
  }
  return thermo::Nasa7(data,
                       pluss::series::make(300.0).to(ScalarType::Float64));
}

template <class Fn>
double median_ms(Fn &&fn, std::int64_t warmup, std::int64_t rounds,
                 double &checksum) {
  for (std::int64_t index = 0; index < warmup; ++index)
    checksum += fn().sum().template item<double>();
  std::vector<double> samples;
  samples.reserve(static_cast<std::size_t>(rounds));
  for (std::int64_t index = 0; index < rounds; ++index) {
    const auto start = Clock::now();
    checksum += fn().sum().template item<double>();
    const auto stop = Clock::now();
    samples.push_back(
        std::chrono::duration<double, std::milli>(stop - start).count());
  }
  std::sort(samples.begin(), samples.end());
  return samples[samples.size() / 2];
}

Series page_eval(const thermo::Nasa7 &nasa, const Series &temperature) {
  std::vector<Series> pages;
  pages.reserve(static_cast<std::size_t>(temperature.size(0)));
  for (std::int64_t page = 0; page < temperature.size(0); ++page)
    pages.push_back(nasa.eval(temperature.select(0, page)).cp_r);
  return pluss::pile(pluss::StackArray<Series>(std::move(pages)).data(), 0);
}

} // namespace

int main() {
  constexpr std::int64_t warmup = 2;
  constexpr std::int64_t rounds = 9;
  const auto nasa = model(12);
  const Series temperature =
      pluss::series::linspace(250.0, 4500.0, 16 * 16 * 8 * 4,
                              ScalarType::Float64)
          .reshape({16, 16, 8, 4});

  const Series batched = nasa.eval(temperature).cp_r;
  const Series paged = page_eval(nasa, temperature);
  if (!batched.allclose(paged, 1e-12, 1e-12)) {
    std::cerr << "batched and page-wise results differ\n";
    return 1;
  }

  double checksum = 0.0;
  const double batch_ms = median_ms([&] { return nasa.eval(temperature).cp_r; },
                                    warmup, rounds, checksum);
  const double page_ms = median_ms([&] { return page_eval(nasa, temperature); },
                                   warmup, rounds, checksum);

  std::cout << std::fixed << std::setprecision(3)
            << "shape=[16,16,8,4] species=12\n"
            << "batched_median_ms=" << batch_ms << '\n'
            << "page_median_ms=" << page_ms << '\n'
            << "dispatch_speedup=" << page_ms / batch_ms << "x\n"
            << "checksum=" << checksum << '\n';
  return 0;
}
