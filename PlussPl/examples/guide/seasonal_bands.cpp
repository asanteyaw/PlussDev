// The typical year with its uncertainty: the mean passengers per calendar month with error bars, a band and a point plot, from twelve years of data.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace pluss::pl;

static std::string out(const char *name) {
  const char *dir = std::getenv("PLUSSMA_GUIDE_ASSETS");
  return ((dir ? std::filesystem::path(dir) : std::filesystem::path(".")) / name).string();
}


int main() {
  auto loaded = flash::load_dataset("flights");
  const auto flights = loaded->frame();
  auto passengers = flights.features("passengers").as_series();
  const auto n = passengers.numel();
  auto calendar_month = pluss::series::arange(0, n, pluss::ScalarType::Int64).remainder(12);
  auto mean = passengers.grouped_mean(calendar_month, 12);
  auto sd = passengers.grouped_std(calendar_month, 12);
  auto x = pluss::series::arange(1, 13, pluss::ScalarType::Float64);
  std::cout << "peak month " << mean.argmax().item<std::int64_t>() + 1 << " with " << mean.max().item<double>() << " passengers; trough month " << mean.argmin().item<std::int64_t>() + 1 << " with " << mean.min().item<double>() << "\n";
  std::cout << "largest spread in month " << sd.argmax().item<std::int64_t>() + 1 << " (std " << sd.max().item<double>() << ")\n";

  auto fig = studio("seasonal_bands");
  fig.mosaic(2, 2);
  fig.headline("The typical year");
  auto &bars = fig.pane2("Native whisk: mean with error bars");
  bars.whisk(x, mean, sd);
  auto &flash_bars = fig.pane2("flash::whiskplot");
  flash::whiskplot(flash_bars, x, mean, sd);
  auto &points = fig.pane2("Point plot");
  flash::pointplot(points, x, mean, sd);
  auto &band = fig.pane2("Ribbon between the mean minus and plus one std");
  band.ribbon(x, mean + sd, 0.0);
  band.trend(x, mean);
  band.trend(x, mean - sd);
  fig.show();
}
