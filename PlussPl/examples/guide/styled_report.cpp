// A report-quality figure from monthly passenger data: a theme, labelled axes, chosen limits and ticks, a legend, and the same figure exported as SVG, PNG and PDF.
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
  auto loaded = flash::load_dataset("flights");
  const auto flights = loaded->frame();
  auto passengers = flights.features("passengers").as_series();
  auto fare = flights.features("fare_index").as_series();
  auto month = pluss::series::arange(0, passengers.numel(), pluss::ScalarType::Float64);
  std::cout << passengers.numel() << " months, passengers from " << passengers.min().item<double>() << " to " << passengers.max().item<double>() << "\n";

  // 1. A theme is chosen when the figure is made; it applies to every pane created afterwards.
  Studio fig("styled_report", Theme::ocean());
  fig.size(1000.0, 520.0);
  fig.headline("Passengers and fares");
  auto &pane = fig.pane2("Monthly passengers");

  // 2. Two traces on one pane, each with a label for the legend and its own line width.
  pane.trend(month, passengers).label("passengers").width(2.0f);
  pane.trend(month, fare * (passengers.mean().item<double>() / fare.mean().item<double>())).label("fare index (rescaled)").width(1.0f);

  // 3. Axes: a label, explicit limits, a tick format, a grid style and a legend position.
  pane.x_label("month").y_label("passengers");
  pane.x_limits({0.0, passengers.numel() - 1.0});
  flash::set_style(pane, GridStyle::MajorMinor);
  pane.legend(KeyPlace::NorthWest);
  flash::legend_frame(pane, KeyFrame::Soft);
  flash::move_legend(pane, KeyPlace::NorthEast);
  std::cout << "figure " << fig.extent().width << " x " << fig.extent().height << ", panes " << fig.pane_count() << "\n";

  // 4. One figure, three formats.
  fig.save_svg(out("styled_report.svg"));
  fig.save_png(out("styled_report.png"), 144);
  fig.save_pdf(out("styled_report.pdf"));
  fig.show();   // opens a window; the documentation build captures the figure instead
  std::cout << "saved SVG, PNG and PDF: " << std::filesystem::exists(out("styled_report.svg")) << std::filesystem::exists(out("styled_report.png")) << std::filesystem::exists(out("styled_report.pdf")) << "\n";
}
