// A one-page sales dashboard from ten years of daily retail data: revenue over time, revenue by region, promotion effect and the shape of daily revenue.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace pluss::pl;
using pluss::table::DataFrame;

static std::filesystem::path assets() {
  const char *dir = std::getenv("PLUSSMA_GUIDE_ASSETS");
  return dir ? std::filesystem::path(dir) : std::filesystem::path(".");
}

int main() {
  auto loaded = flash::load_dataset("retail_daily");
  const DataFrame sales = loaded->frame();
  std::cout << "rows " << sales.ncases() << ", features " << sales.nfeatures() << "\n";

  // 1. Revenue by region: group, sum, and read the totals back as plain Series for plotting.
  auto by_region = sales.group({.dropnull = true, .sort = true}).by({"region"}).agg({{"revenue", "revenue", DataFrame::GroupOp::Sum}});
  std::cout << by_region.to_string() << "\n";

  // 2. One figure, four panes.
  auto fig = studio("retail_dashboard");
  fig.mosaic(2, 2);
  fig.headline("Retail dashboard");
  auto revenue = sales.features("revenue").as_series();
  auto &trend = fig.pane2("Daily revenue");
  trend.trend(pluss::series::arange(0, 365, pluss::ScalarType::Float64), revenue.slice(0, 0, 365)).label("first year");
  auto &regions = fig.pane2("Revenue by region");
  regions.columns(pluss::series::arange(0, by_region.ncases(), pluss::ScalarType::Float64), by_region.features("revenue").as_series());
  auto &promo = fig.pane2("Promotion spend vs revenue");
  promo.cloud(sales.features("promo_spend").as_series().slice(0, 0, 500), revenue.slice(0, 0, 500));
  auto &dist = fig.pane2("Daily revenue distribution");
  dist.bucket(revenue, 30);
  fig.show();
  std::cout << "saved retail_dashboard.svg\n";
}
