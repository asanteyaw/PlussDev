#include <pluss/pluss.h>

#include <cmath>
#include <filesystem>
#include <vector>

int main() {
  using namespace pluss::pl;
  using namespace pluss::pl::flash;

  std::filesystem::create_directories("outputs");

  Studio fig("flash_seaborn_demo", Theme::paper());
  auto& pane = fig.pane2("flash facade");
  set_context(fig, 1200.0, 780.0, 1.2, 1.1);
  set_style(pane, GridStyle::Major);
  move_legend(pane, KeyPlace::OutsideEast);

  auto x = pluss::linspace(0.0, 10.0, 48, pluss::ScalarType::Float64);
  const std::vector<double> xv = x.to_vec<double>();
  std::vector<double> yv;
  yv.reserve(xv.size());
  for (const double value : xv) {
    yv.push_back(std::sin(value * 0.7) + 0.15 * std::cos(value * 2.1));
  }
  auto y = pluss::make_series(yv, {static_cast<std::int64_t>(yv.size())});

  scatterplot(pane, x, y).label("sns.scatterplot");
  lineplot(pane, x, y).label("sns.lineplot");
  regplot(pane, x, y).label("sns.regplot");
  histplot(pane, y, 14).label("sns.histplot");
  kdeplot(pane, y, 96).label("sns.kdeplot");
  ecdfplot(pane, y).label("sns.ecdfplot");

  auto bands = pluss::make_note({"low", "mid", "high", "mid", "low", "high", "mid", "low"});
  auto vals = pluss::make_series(std::vector<double>{0.35, 0.6, 1.15, 0.82, 0.44, 1.22, 0.75, 0.38}, {8});
  stripplot(pane, bands, vals).label("sns.stripplot");
  countplot(pane, bands).label("sns.countplot");

  fig.save_svg("outputs/plusspl_flash_seaborn_demo.svg");
  fig.save_pdf("outputs/plusspl_flash_seaborn_demo.pdf");
  return 0;
}
