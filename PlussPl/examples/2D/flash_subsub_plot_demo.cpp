#include <pluss/pluss.h>

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

int main() {
  using namespace pluss::pl;
  using namespace pluss::pl::flash;

  const auto output_dir = std::filesystem::path(PLUSSPL_EXAMPLES_SOURCE_DIR) / "outputs" / "2D";
  std::filesystem::create_directories(output_dir);
  const auto out_path = [&output_dir](std::string_view name) {
    return (output_dir / std::string(name)).string();
  };

  const auto x = pluss::linspace(0.0, 10.0, 140, pluss::ScalarType::Float64);
  const auto y_base = x.mul(0.55).sin().add(x.mul(1.3).cos().mul(0.2));

  Studio fig("flash_subsub_plot_demo", Theme::paper());
  set_theme(fig, FlashThemeOptions{
                     .style = FlashStyle::Sandgrid,
                     .context = FlashContext::Talk,
                     .width = 1960.0,
                     .height = 1280.0,
                 });
  fig.mosaic(2, 2, 20.0, 20.0);

  const std::array<std::string, 4> cell_titles{
      "cell 0,0: baseline branch",
      "cell 0,1: comparison branch",
      "cell 1,0: shifted branch A",
      "cell 1,1: shifted branch B",
  };
  std::vector<std::size_t> root_indices;
  root_indices.reserve(4);
  std::size_t cell_index = 0;
  for (const auto row : std::views::iota(std::int64_t{0}, std::int64_t{2})) {
    for (const auto col : std::views::iota(std::int64_t{0}, std::int64_t{2})) {
      auto& root = fig.pane2(cell_titles[cell_index++]);
      root.place(row, col).axes_visible(false).grid(GridStyle::None).x_tick_labels(false).y_tick_labels(false);
      root_indices.push_back(fig.pane_index(root));
    }
  }

  for (const auto idx : std::views::iota(std::size_t{0}, root_indices.size())) {
    const auto parent = root_indices[idx];
    const double phase = 0.18 * static_cast<double>(idx + 1);
    const auto y1 = y_base.add(phase);
    const auto y2 = x.mul(0.42 + 0.06 * static_cast<double>(idx)).cos().mul(0.65).add(phase * 0.7);
    const auto y3 = y1.add(pluss::randn({x.numel()}, pluss::ScalarType::Float64, 1234ULL + idx).mul(0.08));

    auto& p00 = fig.subpane2(parent, 4, 2, 0, 0, 2, 1);
    p00.grid(GridStyle::Major).x_label("x").y_label("y");
    lineplot(p00, x, y1).label("series A");
    lineplot(p00, x, y2).label("series B");

    auto& p01 = fig.subpane2(parent, 4, 2, 0, 1, 2, 1);
    p01.grid(GridStyle::Major).x_label("x").y_label("signal");
    scatterplot(p01, x, y3);

    auto& p10 = fig.subpane2(parent, 4, 2, 2, 0, 2, 1);
    p10.grid(GridStyle::Major).x_label("value").y_label("density");
    histplot(p10, y3, 18);
    kdeplot(p10, y3, 80);

    auto& p11 = fig.subpane2(parent, 4, 2, 2, 1, 2, 1);
    p11.grid(GridStyle::Major).x_label("group").y_label("spread");
    const auto groups = pluss::make_note({"A", "A", "A", "A", "B", "B", "B", "B"});
    const auto vals = pluss::make_series(
        std::vector<double>{
            0.2 + phase, 0.4 + phase, 0.6 + phase, 0.5 + phase,
            0.35 + phase, 0.7 + phase, 0.55 + phase, 0.8 + phase,
        },
        {8});
    boxplot(p11, groups, vals);

    if (idx == 0) {
      p11.hide();
      const auto nested_parent = fig.pane_index(p11);
      auto& n0 = fig.subpane2(nested_parent, 1, 1, 0, 0, 1, 1);
      n0.grid(GridStyle::Major).x_label("x").y_label("signal");
      lineplot(n0, x, y1.mul(0.72));
      pointplot(
          n0,
          std::array<double, 6>{0, 1, 2, 3, 4, 5},
          std::array<double, 6>{0.2 + phase, 0.3 + phase, 0.55 + phase, 0.46 + phase, 0.62 + phase, 0.7 + phase},
          std::array<double, 6>{0.05, 0.04, 0.06, 0.05, 0.04, 0.05});
    }
  }

  fig.headline("Subsub plots: 2x2 top cells, each with local 2x2; one branch nested deeper");
  // fig.save_svg(out_path("plusspl_flash_subsub_plot_demo.svg"));
  fig.save_pdf(out_path("plusspl_flash_subsub_plot_demo.pdf"));
  return 0;
}
