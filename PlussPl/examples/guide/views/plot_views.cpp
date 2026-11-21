// The views behind the plot window's menus, driven from code: which groups a figure offers, and what a set of choices does to it.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>

using namespace pluss::pl;

static std::string out(const char *name) {
  const char *dir = std::getenv("PLUSSMA_GUIDE_ASSETS");
  return ((dir ? std::filesystem::path(dir) : std::filesystem::path(".")) / name).string();
}

// A small deterministic generator, so that the program prints the same numbers every time.
static double next_uniform(unsigned long long &state) {
  state = state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>((state >> 33) & 0xFFFFFF) / 16777216.0;
}

static double next_normal(unsigned long long &state) {
  double sum = 0.0;
  for (int i = 0; i < 12; ++i) sum += next_uniform(state);
  return sum - 6.0;
}

static void list_groups(const char *name, const Studio &figure) {
  std::cout << name << " offers:\n";
  for (const auto &group : views::groups(figure)) {
    std::cout << "  " << (group.section.empty() ? "(own menu)" : group.section) << " / " << group.title << ": ";
    for (std::size_t i = 0; i < group.options.size(); ++i) {
      std::cout << (i ? ", " : "") << group.options[i].id;
    }
    std::cout << "\n";
  }
}

static std::vector<double> first_values(const Studio &figure, std::size_t trace, std::size_t count) {
  const auto &data = std::get<TrendData>(figure.panes()[0].traces()[trace].payload());
  const auto all = data.y.to(pluss::ScalarType::Float64).host<double>().array().data;
  return {all.begin(), all.begin() + static_cast<std::ptrdiff_t>(std::min(count, all.size()))};
}

int main() {
  unsigned long long state = 2024;

  // Four prices that start at different levels and wander.
  Studio prices("prices");
  prices.headline("Four prices");
  Pane &pane = prices.pane2();
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
    pane.trend(day, pluss::make_series(y, {120})).label(names[k]);
  }
  pane.legend(KeyPlace::NorthWest).x_label("day").y_label("price");
  list_groups("the price figure", prices);

  // A set of choices rewrites a copy of the figure; the figure itself is never changed.
  views::Choices smooth_index{{"transform", "index"}, {"smooth", "ma7"}};
  Studio indexed = views::apply(prices, smooth_index);
  const auto raw = first_values(prices, 1, 3);
  const auto shown = first_values(indexed, 1, 3);
  std::cout << "\nBeta, first three values as drawn:      " << raw[0] << " " << raw[1] << " " << raw[2] << "\n";
  std::cout << "Beta after Index and a 7-point average: " << shown[0] << " " << shown[1] << " " << shown[2] << "\n";
  std::cout << "y axis label is now: " << indexed.panes()[0].y_axis().label << "\n";

  prices.save_svg(out("views_prices.svg"));
  indexed.save_svg(out("views_prices_indexed.svg"));
  views::apply(prices, {{"layout", "stack"}}).save_svg(out("views_prices_stack.svg"));
  views::apply(prices, {{"focus", "t1"}, {"guides", "sd1"}, {"marks", "extremes"}, {"theme", "midnight"}})
      .save_svg(out("views_prices_focus.svg"));
  std::cout << "\nOne panel per series gives " << views::apply(prices, {{"layout", "stack"}}).pane_count() << " panes.\n";

  // A histogram has no series to smooth, but it has bins, a scale for its bars, and a curve.
  Studio scores("scores");
  std::vector<double> values(600);
  for (auto &v : values) v = 50.0 + 10.0 * next_normal(state);
  scores.pane2().bucket(pluss::make_series(values, {600}), 14).label("score");
  list_groups("\nthe histogram", scores);
  Studio curved = views::apply(scores, {{"bins", "auto"}, {"counts", "density"}, {"curve", "kde"}});
  const auto &bars = std::get<BucketData>(curved.panes()[0].traces()[0].payload());
  std::cout << "Automatic bins chose " << bars.bins << " bins; the curve is the trace named '"
            << curved.panes()[0].traces()[1].label_text() << "'.\n";
  scores.save_svg(out("views_scores.svg"));
  curved.save_svg(out("views_scores_curve.svg"));

  // A scatter offers a fit, outliers and density shading.
  Studio cloud("cloud");
  std::vector<double> x(900), y(900);
  for (std::size_t i = 0; i < x.size(); ++i) {
    x[i] = 10.0 + 3.0 * next_normal(state);
    y[i] = 2.0 * x[i] + 5.0 * next_normal(state) + (i % 97 == 0 ? 40.0 : 0.0);
  }
  cloud.pane2().cloud(pluss::make_series(x, {900}), pluss::make_series(y, {900})).label("points");
  Studio fitted = views::apply(cloud, {{"outliers", "highlight"}, {"fit", "linear"}, {"shade", "on"}});
  std::cout << "\nThe scatter with outliers set aside and a fit has " << fitted.panes()[0].trace_count() << " traces:";
  for (const auto &trace : fitted.panes()[0].traces()) {
    std::cout << " '" << trace.label_text() << "'";
  }
  std::cout << "\n";
  cloud.save_svg(out("views_cloud.svg"));
  fitted.save_svg(out("views_cloud_fit.svg"));
  return 0;
}
