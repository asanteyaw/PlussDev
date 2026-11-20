// Read a seasonal series four ways: as a line, as steps, as a filled area, and as spikes from a baseline; then relate it to the fare.
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
  auto fare = flights.features("fare_index").as_series();
  auto delay = flights.features("delay_minutes").as_series();
  auto season = flights.features("season").as_note();
  const auto n = passengers.numel();
  auto month = pluss::series::arange(0, n, pluss::ScalarType::Float64);

  auto fig = studio("flights_trends");
  fig.mosaic(2, 3);
  fig.headline("One series, six views");
  auto &line = fig.pane2("Line, coloured by season");
  flash::lineplot(line, month, passengers, season);
  auto &step = fig.pane2("Steps");
  flash::stepplot(step, month, passengers);
  auto &area = fig.pane2("Filled area");
  flash::areaplot(area, month, passengers);
  auto &stem = fig.pane2("Delay stems from the mean");
  stem.spike(month, delay, delay.mean().item<double>());
  auto &ribbon = fig.pane2("Ribbon between fares");
  ribbon.ribbon(month, fare, fare.min().item<double>());
  auto &rel = fig.pane2("Passengers against fare");
  flash::relplot(rel, fare, passengers, "scatter");
  fig.show();

  // Numbers from the same series: the yearly growth and the busiest month of the final year.
  auto last_year = passengers.slice(0, n - 12, n);
  auto first_year = passengers.slice(0, 0, 12);
  std::cout << "mean passengers: first year " << first_year.mean().item<double>() << ", last year " << last_year.mean().item<double>() << "\n";
  std::cout << "busiest month of the last year: " << last_year.argmax().item<std::int64_t>() + 1 << "\n";
  std::cout << "correlation of passengers with fare: " << ((passengers - passengers.mean()) * (fare - fare.mean())).sum().item<double>() / std::sqrt((passengers - passengers.mean()).square().sum().item<double>() * (fare - fare.mean()).square().sum().item<double>()) << "\n";
}
