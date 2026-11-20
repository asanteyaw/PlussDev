// Does a longer commute mean a longer time, and by how much per mode? A linear-model figure, a grid of category plots, and the three grid
// constructors filled by hand.
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
  auto loaded = flash::load_dataset("commute_modes");
  const auto survey = loaded->frame();
  auto distance = survey.features("distance_km").as_series();
  auto duration = survey.features("duration_min").as_series();
  auto mode = survey.features("mode").as_note();
  auto delay = survey.features("delay_min").as_series();
  const double dx = distance.mean().item<double>(), dy = duration.mean().item<double>();
  const double slope = ((distance - dx) * (duration - dy)).sum().item<double>() / (distance - dx).square().sum().item<double>();
  std::cout << "minutes per kilometre across all modes: " << slope << "\n";

  // 1. lmplot builds a whole figure: scatter, fitted line and a band.
  auto lm = studio("commute_lm");
  flash::lmplot(lm, distance, duration, "Duration against distance");
  lm.show();

  // 2. catgridplot: one category plot per grouping, in a grid.
  auto catgrid = studio("commute_catgrid");
  flash::catgridplot(catgrid, {mode, mode}, {duration, delay});
  catgrid.show();

  // 3. The grid constructors lay out panes for you to fill: FacetGrid (rows x columns), PairGrid (a square of variables) and JointGrid (joint plus marginals).
  auto facet = studio("commute_facet_grid");
  flash::FacetGrid(facet, 1, 2);
  facet.pane2("duration").bucket(duration, 20);
  facet.pane2("delay").bucket(delay, 20);
  facet.show();
  auto pair = studio("commute_pair_grid");
  flash::PairGrid(pair, 2);
  pair.pane2("d vs d").bucket(distance, 15);
  pair.pane2("d vs t").cloud(duration, distance);
  pair.pane2("t vs d").cloud(distance, duration);
  pair.pane2("t vs t").bucket(duration, 15);
  pair.show();
  auto joint = studio("commute_joint_grid");
  flash::JointGrid(joint);
  joint.pane2("joint").cloud(distance, duration);
  joint.pane2("top").bucket(distance, 20);
  joint.pane2("right").bucket(duration, 20);
  joint.show();
  std::cout << "five figures written\n";
}
