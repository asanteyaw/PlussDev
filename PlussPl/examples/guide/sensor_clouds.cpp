// Where do the machines' sensor readings live? Three sensors (01, 30 and 20) as a 3D point cloud, a trajectory through them, and the density of the cloud as
// nested level surfaces.
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
  auto loaded = flash::load_dataset("sensor64");
  const auto sensors = loaded->frame();
  auto x = sensors.features("sensor_01").as_series();
  auto y = sensors.features("sensor_30").as_series();
  auto z = sensors.features("sensor_20").as_series();
  auto machine = sensors.features("machine").as_note();
  std::cout << x.numel() << " readings; ranges: x " << x.min().item<double>() << ".." << x.max().item<double>() << ", y " << y.min().item<double>() << ".." << y.max().item<double>() << ", z " << z.min().item<double>() << ".." << z.max().item<double>() << "\n";

  auto fig = studio("sensor_clouds");
  fig.mosaic(2, 2);
  fig.headline("Sensors 01, 30 and 20");
  auto &cloud = fig.pane3("Cloud");
  cloud.cloud3(x, y, z);
  auto &sized = fig.pane3("Cloud sized and coloured by sensor 4");
  sized.cloud3(x, y, z, sensors.features("sensor_04").as_series().abs() + 0.5, sensors.features("sensor_05").as_series());
  auto &scatter = fig.pane3("scatter3D");
  flash::scatter3D(scatter, x, y, z);
  auto &path = fig.pane3("The first 200 readings as a path");
  path.path3(x.slice(0, 0, 200), y.slice(0, 0, 200), z.slice(0, 0, 200));
  fig.show();

  // Density: count the readings in an 8 x 8 x 8 grid and draw nested surfaces of equal count.
  const int g = 8;
  auto scale = [&](const pluss::Series &v) { return ((v - v.min()) / (v.max() - v.min() + 1e-9) * g).floor().to(pluss::ScalarType::Int64); };
  auto cell = scale(x) * (g * g) + scale(y) * g + scale(z);
  auto density = pluss::series::zeros({g * g * g}, pluss::ScalarType::Float64).scatter_add(0, cell, pluss::series::ones({x.numel()}, pluss::ScalarType::Float64)).reshape({g, g, g});
  std::cout << "densest cell holds " << density.max().item<double>() << " readings; occupied cells " << density.gt(0.0).sum().item<double>() << " of " << g * g * g << "\n";
  auto shells = studio("sensor_density");
  auto &pane = shells.pane3("Density shells");
  pane.isoshell(density, 4);
  shells.show();

  // A time-ordered view: the same three readings as a 3D line through time, with the plot3D alias.
  auto line = studio("sensor_plot3d");
  auto &lp = line.pane3("plot3D");
  flash::plot3D(lp, x.slice(0, 0, 300), y.slice(0, 0, 300), z.slice(0, 0, 300));
  line.show();
}
