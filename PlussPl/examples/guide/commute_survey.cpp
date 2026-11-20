// What do commuters choose, and what does it cost them? A survey of travel modes shown as counts, shares and per-mode summaries.
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
  auto mode = survey.features("mode").as_note();
  auto duration = survey.features("duration_min").as_series();
  auto cost = survey.features("cost").as_series();
  auto city = survey.features("city").as_note();
  std::cout << survey.ncases() << " respondents\n";
  auto per_mode = survey.group({.dropnull = true, .sort = true}).by({"mode"}).agg({{"people", "duration_min", pluss::table::DataFrame::GroupOp::Count}, {"minutes", "duration_min", pluss::table::DataFrame::GroupOp::Mean}, {"cost", "cost", pluss::table::DataFrame::GroupOp::Mean}});
  std::cout << per_mode.to_string() << "\n";

  auto fig = studio("commute_survey");
  fig.mosaic(2, 3);
  fig.headline("How we commute");
  auto &count = fig.pane2("Respondents per mode");
  flash::countplot(count, mode);
  auto &share = fig.pane2("Share of respondents");
  flash::pieplot(share, per_mode.features("people").as_series());
  auto &wedge = fig.pane2("Native wedge trace");
  wedge.wedge(per_mode.features("people").as_series());
  auto &avg = fig.pane2("Mean duration by mode");
  flash::barplot(avg, per_mode.features("mode").as_note(), per_mode.features("minutes").as_series());
  auto &horiz = fig.pane2("Mean cost, horizontal");
  horiz.columns_h(per_mode.features("cost").as_series());
  auto &cat = fig.pane2("Cost per mode (catplot)");
  flash::catplot(cat, mode, cost, "box");
  fig.show();
  std::cout << "cities: " << city.unique().numel() << ", modes: " << mode.unique().numel() << "\n";
}
