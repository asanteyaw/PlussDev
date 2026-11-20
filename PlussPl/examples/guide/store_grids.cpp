// Small multiples: the same chart for every group, laid out on a grid, and the grid helpers that fill the panes for you.
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
  auto loaded = flash::load_dataset("retail_daily");
  const auto sales = loaded->frame();
  auto revenue = sales.features("revenue").as_series();
  auto footfall = sales.features("footfall").as_series();
  auto orders = sales.features("orders").as_series();
  auto promo = sales.features("promo_spend").as_series();
  std::cout << "series of " << revenue.numel() << " days\n";

  // 1. FacetGrid: a 2 x 2 grid of panes you fill yourself; here, one chart per calendar quarter of the first year.
  auto facets = studio("grids_facet");
  flash::facet_map_plot(facets, 2, 2, [&](Pane &pane, std::int64_t row, std::int64_t col) {
    const auto quarter = row * 2 + col;
    pane.trend(pluss::series::arange(0, 91, pluss::ScalarType::Float64), revenue.slice(0, quarter * 91, quarter * 91 + 91));
  });
  facets.show();

  // 2. PairGrid: every variable against every other; the mapper says what to draw in each cell and whether it is on the diagonal.
  auto pair = studio("grids_pair");
  std::vector<pluss::Series> vars{revenue.slice(0, 0, 500), footfall.slice(0, 0, 500), orders.slice(0, 0, 500)};
  flash::pairgrid_map_plot(pair, 3, [&](Pane &pane, std::int64_t row, std::int64_t col, bool diagonal) {
    if (diagonal) pane.bucket(vars[row], 15);
    else pane.cloud(vars[col], vars[row]);
  });
  pair.show();

  // 3. JointGrid: a joint view with its two marginals.
  auto joint = studio("grids_joint");
  flash::jointgrid_map_plot(joint, [&](Pane &pane) { pane.cloud(promo.slice(0, 0, 600), revenue.slice(0, 0, 600)); },
                            [&](Pane &pane, bool horizontal) { pane.bucket(horizontal ? promo.slice(0, 0, 600) : revenue.slice(0, 0, 600), 15); });
  joint.show();

  // 4. The ready-made grids: relational, categorical and distribution grids from lists of series.
  auto rel = studio("grids_rel");
  flash::relgridplot(rel, {footfall.slice(0, 0, 300), orders.slice(0, 0, 300)}, {revenue.slice(0, 0, 300), revenue.slice(0, 0, 300)}, "scatter");
  rel.show();
  auto dis = studio("grids_dis");
  flash::disgridplot(dis, {revenue.slice(0, 0, 600), footfall.slice(0, 0, 600)}, "hist");
  dis.show();
  std::cout << "five grid figures written\n";
}
