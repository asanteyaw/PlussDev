// Two hundred thousand hourly readings turned into a field: temperature by month and hour of day, drawn as a heat map, contour lines,
// filled bands, and the daily demand cycle as a polar curve.
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
  auto loaded = flash::load_dataset("weather_hourly");
  const auto weather = loaded->frame();
  auto temp = weather.features("temp_c").as_series();
  auto demand = weather.features("demand_mw").as_series();
  auto when = weather.features("timestamp").as_timeline();
  auto month = when.month().to(pluss::ScalarType::Int64) - 1;
  auto hour = when.hour().to(pluss::ScalarType::Int64);
  std::cout << temp.numel() << " readings, mean temperature " << temp.mean().item<double>() << "\n";

  // Mean temperature in each (month, hour) cell: add every reading into its cell, then divide by the cell counts.
  auto cell = month * 24 + hour;
  auto sums = pluss::series::zeros({12 * 24}, pluss::ScalarType::Float64).scatter_add(0, cell, temp);
  auto counts = pluss::series::zeros({12 * 24}, pluss::ScalarType::Float64).scatter_add(0, cell, pluss::series::ones({temp.numel()}, pluss::ScalarType::Float64));
  auto field = (sums / counts).reshape({12, 24});
  std::cout << "warmest cell " << field.max().item<double>() << ", coldest " << field.min().item<double>() << "\n";
  auto hours = pluss::series::arange(0, 24, pluss::ScalarType::Float64);
  auto months = pluss::series::arange(1, 13, pluss::ScalarType::Float64);

  auto fig = studio("weather_fields");
  fig.mosaic(2, 2);
  fig.headline("Temperature by month and hour");
  auto &map = fig.pane2("Field map");
  map.fieldmap(hours, months, field);
  auto &lines = fig.pane2("Contour lines");
  lines.contour(hours, months, field, 8);
  auto &bands = fig.pane2("Filled bands");
  bands.isoband(hours, months, field, 8);
  // The daily demand cycle on a 24-hour dial: angle is the hour, radius the mean demand at that hour.
  auto demand_by_hour = demand.grouped_mean(hour, 24);
  auto &dial = fig.pane2("Demand over the day (polar)");
  dial.radial(hours * (2.0 * 3.141592653589793 / 24.0), demand_by_hour);
  fig.show();
  std::cout << "peak demand at hour " << demand_by_hour.argmax().item<std::int64_t>() << ", trough at hour " << demand_by_hour.argmin().item<std::int64_t>() << "\n";
}
