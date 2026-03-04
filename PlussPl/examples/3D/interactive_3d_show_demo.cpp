#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <vector>

#include "matlab_surface_preset.h"
#include "../output_paths.h"
#include "pluss/plot/plot.h"

namespace {

pluss::Config plot_series_cfg() {
  return pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
}

pluss::Series make_numeric_series(const std::vector<double>& values) {
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, plot_series_cfg());
}

pluss::Series make_grid_series(const std::vector<std::vector<double>>& rows) {
  const std::int64_t row_count = static_cast<std::int64_t>(rows.size());
  const std::int64_t col_count = row_count == 0 ? 0 : static_cast<std::int64_t>(rows.front().size());
  std::vector<double> flat;
  flat.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, row_count * col_count)));
  for (const auto& row : rows) {
    flat.insert(flat.end(), row.begin(), row.end());
  }
  return pluss::make_series(flat, {row_count, col_count}, plot_series_cfg());
}

} // namespace

int main() {
  std::vector<double> strike;
  std::vector<double> maturity;
  strike.reserve(17);
  maturity.reserve(12);
  for (std::size_t i = 0; i < 17; ++i) {
    strike.push_back(70.0 + static_cast<double>(i) * 5.0);
  }
  for (std::size_t j = 0; j < 12; ++j) {
    maturity.push_back(0.1 + static_cast<double>(j) * 0.2);
  }

  std::vector<std::vector<double>> iv_surface(maturity.size(), std::vector<double>(strike.size(), 0.0));
  for (std::size_t r = 0; r < maturity.size(); ++r) {
    for (std::size_t c = 0; c < strike.size(); ++c) {
      const double m = maturity[r];
      const double k = strike[c];
      const double moneyness = (k - 100.0) / 30.0;
      iv_surface[r][c] = 0.15 + 0.11 * moneyness * moneyness + 0.035 * std::sqrt(m) + 0.01 * std::sin(2.3 * m + 0.7 * moneyness);
    }
  }

  double zmin = iv_surface.front().front();
  double zmax = iv_surface.front().front();
  for (const auto& row : iv_surface) {
    for (double value : row) {
      zmin = std::min(zmin, value);
      zmax = std::max(zmax, value);
    }
  }
  const double strike_span = strike.back() - strike.front();
  const double maturity_span = maturity.back() - maturity.front();
  const double vol_span = zmax - zmin;
  const auto strike_series = make_numeric_series(strike);
  const auto maturity_series = make_numeric_series(maturity);
  const auto surface_series = make_grid_series(iv_surface);

  auto fig = pluss::plotism::surf(strike_series, maturity_series, surface_series, "IV surface")
                 .with_title("PlussPl Implied Volatility Surface");
  plusspl::examples::apply_matlab_surface_preset(fig, strike_span, maturity_span, vol_span);
  fig.x_label("strike").y_label("maturity").z_label("implied volatility");
  fig.legend(pluss::plotism::Legend{false, pluss::plotism::LegendLocation::TopRight, ""});

  const auto svg = plusspl::examples::output_path(__FILE__, "plusspl_interactive_3d.svg");
  const auto png = plusspl::examples::output_path(__FILE__, "plusspl_interactive_3d.png");
  const auto pdf = plusspl::examples::output_path(__FILE__, "plusspl_interactive_3d.pdf");
  constexpr double export_dpi = 180.0;
  fig.save_svg(svg, 1200, 900, export_dpi);
  fig.save_png(png, 1200, 900, export_dpi);
  fig.save_pdf(pdf, 1200, 900, export_dpi);

  const bool opened = fig.show_window(false);
  std::cout << "show_window() opened viewer: " << (opened ? "true" : "false") << "\n";
  std::cout << "Saved SVG: " << svg << "\n";
  std::cout << "Saved PNG: " << png << "\n";
  std::cout << "Saved PDF: " << pdf << "\n";

  std::cout << "Calling fig.show() now. If interactive show is disabled or viewer unavailable, you will see text preview.\n";
  fig.show();

  return 0;
}
