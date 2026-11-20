// Compare the thickness of manufactured batches across plants: the five ways of showing a distribution per group, and the means behind them.
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
  auto loaded = flash::load_dataset("quality_batches");
  const auto batches = loaded->frame();
  auto thickness = batches.features("thickness").as_series();
  auto plant = batches.features("plant").as_note();
  std::cout << batches.ncases() << " batches\n";
  auto means = batches.group({.dropnull = true, .sort = true}).by({"plant"}).agg({{"batches", "thickness", pluss::table::DataFrame::GroupOp::Count}, {"mean", "thickness", pluss::table::DataFrame::GroupOp::Mean}, {"std", "thickness", pluss::table::DataFrame::GroupOp::Std}});
  std::cout << means.to_string() << "\n";

  auto fig = studio("quality_control");
  fig.mosaic(2, 3);
  fig.headline("Thickness by plant");
  auto &box = fig.pane2("Box plot");
  flash::boxplot(box, plant, thickness);
  auto &boxen = fig.pane2("Boxen plot");
  flash::boxenplot(boxen, thickness);
  auto &violin = fig.pane2("Violin plot");
  flash::violinplot(violin, plant, thickness);
  auto &strip = fig.pane2("Strip plot");
  flash::stripplot(strip, plant, thickness);
  auto &swarm = fig.pane2("Swarm plot");
  flash::swarmplot(swarm, pluss::series::arange(0, thickness.numel(), pluss::ScalarType::Float64).remainder(4.0), thickness);
  auto &quartet = fig.pane2("Native quartet trace");
  quartet.quartet(thickness);
  fig.show();

  // The native violin trace takes numeric group codes and its own options: here a narrower, smoother shape per plant.
  auto codes = pluss::series::arange(0, thickness.numel(), pluss::ScalarType::Float64).remainder(3.0);
  auto native = studio("quality_violin");
  auto &pane = native.pane2("Native violin trace");
  pane.violin(codes, thickness, ViolinOptions{.width = 0.6, .bw_adjust = 0.8, .inner = ViolinInner::Quartile});
  native.show();
  std::cout << "native violin written\n";
}
