#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#include <pluss/pluss.h>
#include <pluss/tils.h>

namespace fs = std::filesystem;

namespace {

fs::path output_dir() {
  return fs::path(PLUSSMIC_SOURCE_DIR) / "outputs" / "kindle_catalog_plot_demo";
}

bool show_plots() {
  if (const char* env = std::getenv("PLUSS_SHOW_PLOTS")) {
    return std::string_view(env) == "1";
  }
  return false;
}

void save_plot(const pluss::pl::Studio& figure, std::string_view stem) {
  fs::create_directories(output_dir());
  const auto pdf_path = output_dir() / (std::string(stem) + ".pdf");
  figure.save_pdf(pdf_path.string());
  std::cout << "saved: " << pdf_path << "\n";
  if (show_plots()) {
    (void)figure.show(stem, 1400, 920, true);
  }
}

pluss::kindle::Stream labeled_series(std::vector<double> values, std::vector<std::string> labels) {
  const auto n = static_cast<std::int64_t>(values.size());
  const std::array<std::int64_t, 1> shape{n};
  auto label = pluss::table::Slate(pluss::make_note(std::move(labels), shape));
  return pluss::kindle::Stream(pluss::make_series(std::move(values), {n}), std::nullopt, std::move(label));
}

pluss::kindle::Catalog make_overlay_catalog() {
  using namespace pluss::kindle::literals;
  return pluss::kindle::Catalog({
      "sales"_k = labeled_series(
          {102.0, 105.0, 103.0, 109.0, 111.0, 115.0},
          {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}),
      "returns"_k = labeled_series(
          {4.0, 5.0, 3.0, 6.0},
          {"Mon", "Tue", "Thu", "Sat"}),
      "inventory"_k = labeled_series(
          {45.0, 44.0, 46.0, 41.0, 40.0},
          {"Mon", "Tue", "Wed", "Fri", "Sat"}),
  });
}

pluss::kindle::Catalog make_pair_catalog() {
  using namespace pluss::kindle::literals;
  return pluss::kindle::Catalog({
      "price"_k = labeled_series(
          {100.0, 101.0, 103.0, 102.0, 106.0, 108.0, 107.0},
          {"d1", "d2", "d3", "d4", "d5", "d6", "d7"}),
      "volume"_k = labeled_series(
          {20.0, 22.0, 19.0, 25.0, 28.0, 24.0},
          {"d1", "d2", "d4", "d5", "d6", "d7"}),
      "spread"_k = labeled_series(
          {1.5, 1.7, 1.6, 1.9, 2.1},
          {"d1", "d3", "d4", "d6", "d7"}),
      "depth"_k = labeled_series(
          {300.0, 280.0, 310.0, 290.0, 275.0, 265.0},
          {"d2", "d3", "d4", "d5", "d6", "d7"}),
  });
}

pluss::kindle::Catalog make_scatter_catalog() {
  using namespace pluss::kindle::literals;
  const std::array<std::int64_t, 1> shape{6};
  return pluss::kindle::Catalog({
      "time"_k = pluss::kindle::Stream(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6})),
      "price"_k = pluss::kindle::Stream(pluss::make_series(std::vector<double>{10.2, 10.5, 10.4, 10.9, 11.1, 11.4}, {6})),
      "desk"_k = pluss::kindle::Stream(pluss::make_note(std::vector<std::string>{"A", "A", "B", "B", "A", "B"}, shape)),
  });
}

} // namespace

int main() {
  const auto overlay = make_overlay_catalog();
  const auto pairwise = make_pair_catalog();
  const auto scatter = make_scatter_catalog();

  save_plot(overlay.plot().line({"sales", "returns", "inventory"}), "catalog_line_ragged");
  save_plot(overlay.plot().step({"sales", "returns", "inventory"}), "catalog_step_ragged");
  save_plot(overlay.plot().area({"sales", "returns", "inventory"}), "catalog_area_ragged");
  save_plot(overlay.plot().bar({"sales", "returns"}), "catalog_bar_ragged");
  save_plot(overlay.plot().hist({"sales", "returns", "inventory"}, 12), "catalog_hist_overlay");
  save_plot(overlay.plot().density({"sales", "returns", "inventory"}, 128), "catalog_density_overlay");
  save_plot(scatter.plot().scatter("time", "price", "desk"), "catalog_scatter_hue");
  save_plot(pairwise.plot().scatter_matrix({"price", "volume", "spread", "depth"}), "catalog_scatter_matrix_pairwise");
  save_plot(pairwise.plot().corr_heatmap({"price", "volume", "spread", "depth"}), "catalog_corr_heatmap_pairwise");
  save_plot(overlay.plot().auto_plot(), "catalog_auto_plot");

  std::cout << "catalog plot demo complete\n";
  return 0;
}
