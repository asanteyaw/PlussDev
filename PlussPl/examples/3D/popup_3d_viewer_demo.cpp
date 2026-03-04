#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstddef>
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
  std::vector<double> expiry;
  strike.reserve(17);
  expiry.reserve(11);
  for (std::size_t i = 0; i < 17; ++i) {
    strike.push_back(75.0 + static_cast<double>(i) * 4.0);
  }
  for (std::size_t j = 0; j < 11; ++j) {
    expiry.push_back(0.15 + static_cast<double>(j) * 0.22);
  }

  std::vector<std::vector<double>> iv(expiry.size(), std::vector<double>(strike.size(), 0.0));
  for (std::size_t r = 0; r < expiry.size(); ++r) {
    for (std::size_t c = 0; c < strike.size(); ++c) {
      const double t = expiry[r];
      const double k = strike[c];
      const double x = (k - 102.0) / 26.0;
      iv[r][c] = 0.16 + 0.09 * x * x + 0.028 * std::sqrt(t) + 0.008 * std::cos(2.0 * t + 1.2 * x);
    }
  }

  double zmin = iv.front().front();
  double zmax = iv.front().front();
  for (const auto& row : iv) {
    for (double value : row) {
      zmin = std::min(zmin, value);
      zmax = std::max(zmax, value);
    }
  }
  const double strike_span = strike.back() - strike.front();
  const double expiry_span = expiry.back() - expiry.front();
  const double vol_span = zmax - zmin;
  const auto strike_series = make_numeric_series(strike);
  const auto expiry_series = make_numeric_series(expiry);
  const auto iv_series = make_grid_series(iv);

  auto fig = pluss::plotism::surf(strike_series, expiry_series, iv_series, "IV surface")
                 .with_title("PlussPl Implied Volatility Surface");
  plusspl::examples::apply_matlab_surface_preset(fig, strike_span, expiry_span, vol_span);
  fig.x_label("strike").y_label("maturity").z_label("implied volatility");
  fig.legend(pluss::plotism::Legend{false, pluss::plotism::LegendLocation::TopRight, ""});

  const auto svg = plusspl::examples::output_path(__FILE__, "plusspl_popup_3d.svg");
  const auto png = plusspl::examples::output_path(__FILE__, "plusspl_popup_3d.png");
  const auto pdf = plusspl::examples::output_path(__FILE__, "plusspl_popup_3d.pdf");
  constexpr double export_dpi = 180.0;
  fig.save_svg(svg, 1280, 920, export_dpi);
  fig.save_png(png, 1280, 920, export_dpi);
  fig.save_pdf(pdf, 1280, 920, export_dpi);

  std::cout << "Saved SVG: " << svg << "\n";
  std::cout << "Saved PNG: " << png << "\n";
  std::cout << "Saved PDF: " << pdf << "\n";
  std::cout << "Opening blocking popup viewer. Close the window to return.\n";
  const bool opened = fig.show_popup(pluss::plotism::OutputFormat::Svg, 1280, 920);
  std::cout << "show_popup() opened viewer: " << (opened ? "true" : "false") << "\n";

  return 0;
}
