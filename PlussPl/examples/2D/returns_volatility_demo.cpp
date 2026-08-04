#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <string_view>

int main() {
  namespace pl = pluss::pl;

  const auto output_dir = std::filesystem::path(PLUSSPL_EXAMPLES_SOURCE_DIR) / "outputs" / "2D";
  std::filesystem::create_directories(output_dir);
  const auto out_path = [&output_dir](std::string_view name) {
    return (output_dir / std::string(name)).string();
  };

  constexpr std::int64_t samples = 756;
  constexpr double annual_mu = 0.11;
  constexpr double annual_sigma = 0.24;
  constexpr double trading_days = 252.0;
  const double dt = 1.0 / trading_days;
  const double drift = (annual_mu - 0.5 * annual_sigma * annual_sigma) * dt;
  const double shock_scale = annual_sigma * std::sqrt(dt);

  const auto shocks = pluss::series::randn({samples}, pluss::ScalarType::Float64, 20260305ULL);
  const auto log_returns = shocks.mul(shock_scale).add(drift);
  const auto prices = log_returns.exp().cumprod(0).mul(100.0);
  const auto log_prices = prices.log();
  const auto prev_idx = pluss::series::arange(0, log_prices.numel() - 1, 1, pluss::ScalarType::Int64);
  const auto next_idx = pluss::series::arange(1, log_prices.numel(), 1, pluss::ScalarType::Int64);
  const auto returns = log_prices.index_select(0, next_idx).sub(log_prices.index_select(0, prev_idx));
  constexpr std::int64_t vol_window = 21;
  const auto rolling_vol = returns.unfold(0, vol_window, 1).std(1, false, false).mul(std::sqrt(trading_days));
  const auto rolling_mean = returns.unfold(0, vol_window, 1).mean(1, false);
  const auto t = pluss::series::linspace(1.0, static_cast<double>(returns.numel()), returns.numel(), pluss::ScalarType::Float64);
  const auto t_vol = pluss::series::linspace(
      static_cast<double>(vol_window),
      static_cast<double>(returns.numel()),
      rolling_vol.numel(),
      pluss::ScalarType::Float64);
  const auto t_roll = pluss::series::linspace(
      static_cast<double>(vol_window),
      static_cast<double>(returns.numel()),
      rolling_mean.numel(),
      pluss::ScalarType::Float64);
  const auto baseline_vol = pluss::series::full({rolling_vol.numel()}, annual_sigma, pluss::ScalarType::Float64);

  pl::Studio studio("returns-volatility-demo", pl::Theme::paper());
  studio.size(1420.0, 920.0);
  studio.mosaic(2, 1, 16.0, 18.0);
  studio.tight_layout(true, 4.0);
  studio.headline("Random Returns And Rolling Volatility (Vectorized Series)");

  auto& returns_pane = studio.pane2("Daily Log Returns");
  returns_pane.place(0, 0)
      .grid(pl::GridStyle::Major)
      .legend(pl::KeyPlace::Auto)
      .x_label("observation")
      .y_label("return")
      .caption("simulated with normal innovations");
  returns_pane.rule_y(0.0, "zero line");
  returns_pane.trend(t, returns).label("log return").width(1.35F);
  returns_pane.trend(t_roll, rolling_mean).label("rolling mean (21)").width(1.8F);

  auto& vol_pane = studio.pane2("Rolling Volatility");
  vol_pane.place(1, 0)
      .grid(pl::GridStyle::Major)
      .legend(pl::KeyPlace::Auto)
      .x_label("observation")
      .y_label("annualized vol")
      .caption("21-sample moving standard deviation");
  vol_pane.trend(t_vol, rolling_vol).label("rolling vol (21)").width(1.5F);
  vol_pane.trend(t_vol, baseline_vol).label("target vol").width(1.35F);

  studio.save_svg(out_path("plusspl_returns_volatility_demo.svg"));
  studio.save_pdf(out_path("plusspl_returns_volatility_demo.pdf"));
  try {
    studio.save_png(out_path("plusspl_returns_volatility_demo.png"), 160);
  } catch (const std::exception&) {
    // PNG export requires an external SVG rasterizer (rsvg-convert/inkscape/magick).
  }
  
  return 0;
}
