// One figure for each family of views, so the window's menus can be seen at work: every file is the figure the window shows after
// the named choices.
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

static double next_uniform(unsigned long long &state) {
  state = state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>((state >> 33) & 0xFFFFFF) / 16777216.0;
}

static double next_normal(unsigned long long &state) {
  double sum = 0.0;
  for (int i = 0; i < 12; ++i) sum += next_uniform(state);
  return sum - 6.0;
}

static void show(const char *file, const Studio &figure, const views::Choices &choices) {
  views::apply(figure, choices).save_svg(out(file));
  std::cout << file << ":";
  for (const auto &[group, option] : choices) {
    std::cout << " " << group << "=" << option;
  }
  std::cout << "\n";
}

int main() {
  unsigned long long state = 7;

  Studio prices("prices");
  Pane &lines = prices.pane2();
  auto day = pluss::series::linspace(1.0, 120.0, 120, pluss::ScalarType::Float64);
  const char *names[] = {"Alpha", "Beta", "Gamma", "Delta"};
  const double starts[] = {104.0, 115.0, 106.0, 105.0};
  for (int k = 0; k < 4; ++k) {
    std::vector<double> y(120);
    double level = starts[k];
    for (auto &v : y) {
      level += 2.0 * next_normal(state) + 0.2;
      v = level;
    }
    lines.trend(day, pluss::make_series(y, {120})).label(names[k]);
  }
  lines.legend(KeyPlace::NorthWest).x_label("day").y_label("price");

  show("view_smooth.svg", prices, {{"smooth", "ma15"}, {"ghost", "faint"}, {"marks", "extremes"}});
  show("view_index.svg", prices, {{"transform", "index"}});
  show("view_focus.svg", prices, {{"focus", "t1"}, {"guides", "sd1"}});
  show("view_panels.svg", prices, {{"layout", "grid"}, {"transform", "zscore"}});
  show("view_area.svg", prices, {{"isolate", "t2"}, {"draw", "area"}, {"guides", "iqr"}});
  show("view_steps.svg", prices, {{"window", "last25"}, {"draw", "linepoints"}});
  show("view_log.svg", prices, {{"transform", "cumsum"}, {"scale", "ylog"}});
  show("view_midnight.svg", prices, {{"theme", "midnight"}, {"focus", "t0"}});

  Studio regions("regions");
  Pane &bars = regions.pane2();
  bars.columns(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, std::vector<double>{42, 17, 88, 63, 29, 71, 55, 9}).label("sales");
  bars.x_categories({"North", "East", "Atlas", "Zulu", "Mid", "Bravo", "Coast", "Delta"});
  bars.x_label("region").y_label("sales");
  show("view_bars.svg", regions, {{"sort", "desc"}, {"marks", "values"}, {"guides", "mean"}});
  show("view_bars_flipped.svg", regions, {{"orient", "flip"}, {"sort", "asc"}, {"transform", "share"}});

  Studio scores("scores");
  std::vector<double> values(800);
  for (auto &v : values) v = 50.0 + 10.0 * next_normal(state);
  scores.pane2().bucket(pluss::make_series(values, {800}), 14).label("score");
  show("view_hist_curve.svg", scores, {{"bins", "auto"}, {"counts", "density"}, {"curve", "kde"}});
  show("view_hist_cumulative.svg", scores, {{"counts", "cdf"}, {"curve", "normal"}, {"guides", "iqr"}});

  Studio cloud("cloud");
  std::vector<double> x(1200), y(1200);
  for (std::size_t i = 0; i < x.size(); ++i) {
    x[i] = 10.0 + 3.0 * next_normal(state);
    y[i] = 2.0 * x[i] + 5.0 * next_normal(state) + (i % 97 == 0 ? 40.0 : 0.0);
  }
  cloud.pane2().cloud(pluss::make_series(x, {1200}), pluss::make_series(y, {1200})).label("points");
  show("view_scatter_fit.svg", cloud, {{"outliers", "highlight"}, {"fit", "linear"}, {"stats", "summary"}});
  show("view_scatter_shade.svg", cloud, {{"shade", "on"}, {"fit", "curve"}});
  return 0;
}
