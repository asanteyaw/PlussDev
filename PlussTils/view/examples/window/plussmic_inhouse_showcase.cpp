#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <pluss/pluss.h>
#include <pluss/tils/view/html_window.h>

namespace {

pluss::table::DataFrame make_frame(const double scale, const std::string& title) {
  std::vector<double> revenue;
  std::vector<double> margin;
  std::vector<std::int64_t> orders;
  std::vector<std::string> region;
  std::vector<std::string> channel;
  revenue.reserve(28);
  margin.reserve(28);
  orders.reserve(28);
  region.reserve(28);
  channel.reserve(28);

  constexpr const char* regions[] = {"North", "South", "East", "West"};
  constexpr const char* channels[] = {"Direct", "Partner", "Online"};
  for (int row = 0; row < 28; ++row) {
    revenue.push_back(scale * (8200.0 + 540.0 * row + 1100.0 * std::sin(row * 0.42)));
    margin.push_back(0.14 + 0.015 * (row % 7));
    orders.push_back(48 + row * 3);
    region.emplace_back(regions[row % 4]);
    channel.emplace_back(channels[row % 3]);
  }

  return pluss::table::DataFrame({
      {"region", pluss::make_note(region)},
      {"channel", pluss::make_note(channel)},
      {"orders", pluss::make_series(orders)},
      {"revenue", pluss::make_series(revenue)},
      {"margin", pluss::make_series(margin)},
      {"forecast", pluss::make_series(revenue) * 1.08},
      {"variance", pluss::make_series(revenue) * 0.08},
      {"target", pluss::make_series(revenue) * 1.12},
      {"cost", pluss::make_series(revenue) * 0.64},
      {"profit", pluss::make_series(revenue) * 0.36},
      {"conversion", pluss::make_series(margin) * 0.42},
      {"quota", pluss::make_series(orders) * 4.0},
  }).title(title);
}

pluss::table::DataFrame make_plot_frame() {
  std::vector<double> revenue;
  std::vector<double> margin;
  std::vector<std::int64_t> orders;
  revenue.reserve(28);
  margin.reserve(28);
  orders.reserve(28);
  for (int row = 0; row < 28; ++row) {
    revenue.push_back(8200.0 + 540.0 * row + 1100.0 * std::sin(row * 0.42));
    margin.push_back(0.14 + 0.015 * (row % 7));
    orders.push_back(48 + row * 3);
  }
  return pluss::table::DataFrame({
      {"orders", pluss::make_series(orders)},
      {"revenue", pluss::make_series(revenue)},
      {"margin", pluss::make_series(margin)},
      {"forecast", pluss::make_series(revenue) * 1.08},
  }).title("Plot measures");
}

} // namespace

int main(int argc, char** argv) {
  const std::string mode = argc > 1 ? argv[1] : "dataframe";
  const auto frame = make_frame(1.0, "Commercial performance");

  if (mode == "plot") {
    pluss::view::HtmlWindowOptions options;
    options.title = "PlussView Plot Workspace";
    options.snapshot_name = "/tmp/plussview_plot_showcase.png";
    options.svg_snapshot_name = "/tmp/plussview_plot_showcase.svg";
    options.width = 1180;
    options.height = 760;
    options.backend_preference = pluss::view::BackendPreference::PlussWebViewOnly;
    auto figure = make_plot_frame().plot().scatter_matrix();
    figure.headline("Revenue performance and relationships");
    figure.save_svg(options.svg_snapshot_name);
    return figure.show(options) ? 0 : 1;
  }

  pluss::table::display::DisplayOptions options;
  options.backend = pluss::table::display::DisplayBackend::PlussWebView;
  options.viewer_width = 1180;
  options.viewer_height = 760;

  if (mode == "atlas") {
    options.window_title = "Pluss Atlas Workspace";
    const pluss::table::Atlas atlas(
        {
            make_frame(0.92, "Baseline"),
            make_frame(1.00, "Current"),
            make_frame(1.14, "Forecast"),
        },
        pluss::make_note({"Baseline", "Current", "Forecast"}));
    atlas.show(options);
    return 0;
  }

  options.window_title = "Pluss DataFrame Workspace";
  frame.show(options);
  return 0;
}
