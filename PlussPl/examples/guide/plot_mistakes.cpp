// The mistakes that stop a figure from being made, and what the error says: a 3D trace on a 2D pane, data of different lengths, a heat map
// without a matrix, and a dataset that does not exist.
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
  const auto report = [](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      std::cout << e.message() << "\n";
    }
  };
  const std::vector<double> x{0, 1, 2, 3}, y{1, 4, 9, 16}, z{0, 1, 0, 1};
  auto fig = studio("mistakes");

  // 1. Each pane has a dimension. A 3D trace on a 2D pane is refused, and so is a 2D trace on a 3D pane.
  auto &flat = fig.pane2("flat");
  report([&] { flat.sheet(x, x, z); });
  auto &volume = fig.pane3("volume");
  report([&] { volume.trend(x, y); });

  // 2. Data that do not line up are refused, with the lengths named.
  report([&] { flat.trend(x, std::vector<double>{1, 2, 3}); });

  // 3. A field needs a matrix. Three vectors are not a grid.
  report([&] { flash::heatmap(flat, pluss::make_series(x), pluss::make_series(x), pluss::make_series(y)); });

  // 4. A dataset name that does not exist is not an error: load_dataset returns an empty optional, which must be checked.
  auto missing = flash::load_dataset("no_such_dataset");
  std::cout << "load_dataset(\"no_such_dataset\") has a value: " << missing.has_value() << "\n";
}
