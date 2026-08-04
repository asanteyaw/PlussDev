#include <pluss/pluss.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <string>
#include <string_view>

namespace {

struct ModelPaths {
  pluss::Series returns;
  pluss::Series sigma;
};

[[nodiscard]] double scalar_f64(const pluss::Series& value) {
  return pluss::pl::flash::detail::to_f64_vector(value.reshape({1}))[0];
}

[[nodiscard]] pluss::Series stabilize_ngarch_beta(
    const pluss::Series& alpha,
    const pluss::Series& beta_raw,
    const pluss::Series& gamma) {
  const auto phi_target = pluss::series::full(beta_raw.sizes(), 0.985, pluss::ScalarType::Float64);
  const auto beta_cap = phi_target - alpha * (1.0 + gamma.pow(2));
  return beta_raw.clamp(1.0e-5, 0.995).minimum(beta_cap.clamp(1.0e-5, 0.995));
}

ModelPaths simulate_paths(const pluss::Series& omega,
                          const pluss::Series& alpha,
                          const pluss::Series& beta,
                          const pluss::Series& lambda,
                          const pluss::Series& gamma,
                          double risk_free_rate,
                          std::int64_t steps,
                          std::uint64_t seed) {
  const std::int64_t assets = omega.numel();
  ModelPaths out;

  const auto phi = beta + alpha * (1.0 + gamma.pow(2));
  PLUSS_CHECK(scalar_f64(phi.max()) < 1.0, "NGARCH stationarity violated: phi = beta + alpha * (1 + gamma^2) must be < 1");
  auto sigma2_prev = (omega / (1.0 - phi)).clamp(1.0e-8, 0.35);
  auto eps_prev = pluss::series::zeros({assets}, pluss::ScalarType::Float64);
  const auto eps_all = pluss::series::randn({steps, assets}, pluss::ScalarType::Float64, seed);
  auto returns_all = pluss::series::zeros({steps, assets}, pluss::ScalarType::Float64);
  auto sigma_all = pluss::series::zeros({steps, assets}, pluss::ScalarType::Float64);

  const double r_daily = risk_free_rate / 252.0;

  for (std::int64_t t = 0; t < steps; ++t) {
    const auto t_idx = pluss::series::arange(t, t + 1, 1, pluss::ScalarType::Int64);
    const auto eps_t = eps_all.index_select(0, t_idx).reshape({assets});
    const auto scaled_shock = eps_prev + gamma;
    const auto sigma2_t = (omega + beta * sigma2_prev + alpha * sigma2_prev * scaled_shock * scaled_shock)
                              .clamp(1.0e-10, 0.35);
    const auto sigma_t = sigma2_t.sqrt();
    const auto mu_t = r_daily + lambda * sigma_t - 0.5 * sigma2_t;
    const auto y_t = mu_t + sigma_t * eps_t;

    returns_all.index_put_({t_idx}, y_t.reshape({1, assets}), false);
    sigma_all.index_put_({t_idx}, sigma_t.reshape({1, assets}), false);
    sigma2_prev = sigma2_t;
    eps_prev = eps_t;
  }

  out.returns = std::move(returns_all);
  out.sigma = std::move(sigma_all);
  return out;
}

pluss::Series sigma_spread_rms(const ModelPaths& ngarch, const ModelPaths& garch) {
  const auto spread = ngarch.sigma - garch.sigma;
  return (spread * spread).mean(1, false).sqrt();
}

pluss::Series asset_column(const pluss::Series& matrix, std::int64_t asset_index) {
  const auto index = pluss::series::arange(asset_index, asset_index + 1, 1, pluss::ScalarType::Int64);
  return matrix.index_select(1, index).reshape({matrix.sizes()[0]});
}

} // namespace

int main() {
  namespace pl = pluss::pl;
  namespace flash = pluss::pl::flash;

  const auto output_dir = std::filesystem::path(PLUSSPL_EXAMPLES_SOURCE_DIR) / "outputs" / "2D";
  std::filesystem::create_directories(output_dir);
  const auto out_path = [&output_dir](std::string_view name) {
    return (output_dir / std::string(name)).string();
  };

  constexpr std::int64_t steps = 756;
  constexpr double risk_free_rate = 0.19;
  constexpr std::int64_t assets_count = 5;
  const auto assets = pluss::make_note({"Asset 1", "Asset 2", "Asset 3", "Asset 4", "Asset 5"});

  // Grounded near common NGARCH estimates.
  const auto omega = pluss::make_series({1.2037e-4, 1.0500e-4, 1.3200e-4, 1.1100e-4, 1.2600e-4},
                                        {assets_count});
  const auto alpha = pluss::make_series({0.0451, 0.0380, 0.0500, 0.0420, 0.0470}, {assets_count});
  const auto beta_raw = pluss::make_series({0.9200, 0.9280, 0.9120, 0.9180, 0.9250}, {assets_count});
  const auto lambda = pluss::make_series({0.0234, 0.0195, 0.0272, 0.0218, 0.0254}, {assets_count});
  const auto gamma = pluss::make_series({-1.2365, -1.0500, -1.3400, -1.1800, -1.2800}, {assets_count});
  const auto beta = stabilize_ngarch_beta(alpha, beta_raw, gamma);
  const auto gamma_garch = pluss::series::zeros({assets_count}, pluss::ScalarType::Float64);

  const auto ngarch = simulate_paths(omega, alpha, beta, lambda, gamma, risk_free_rate, steps, 2026030501ULL);
  const auto garch = simulate_paths(omega, alpha, beta, lambda, gamma_garch, risk_free_rate, steps, 2026030502ULL);
  const auto spread_rms = sigma_spread_rms(ngarch, garch);
  const auto t = pluss::series::linspace(1.0, static_cast<double>(steps), steps, pluss::ScalarType::Float64);

  pl::Studio studio("ngarch-demo", pl::Theme::paper());
  studio.size(1680.0, 980.0);
  studio.mosaic(2, 3, 18.0, 18.0);
  studio.tight_layout(true, 4.0);
  studio.headline("NGARCH vs GARCH (5 Assets): Returns, Volatility, And Leverage Impact");

  auto& ng_ret = studio.pane2("NGARCH Returns");
  ng_ret.place(0, 0)
      .grid(pl::GridStyle::Major)
      .legend(pl::KeyPlace::Auto)
      .legend_frame(pl::KeyFrame::Soft)
      .x_label("observation")
      .y_label("$y_t$")
      .caption("$\\mu_t = r + \\lambda\\sigma_t - \\frac{1}{2}\\sigma_t^2$");
  for (std::int64_t i = 0; i < assets_count; ++i) {
    flash::lineplot(ng_ret, t, asset_column(ngarch.returns, i))
        .label(std::string(assets.at(i)))
        .width(1.35F);
  }

  auto& ng_vol = studio.pane2("NGARCH Volatility");
  ng_vol.place(0, 1)
      .grid(pl::GridStyle::Major)
      .legend(pl::KeyPlace::Auto)
      .legend_frame(pl::KeyFrame::None)
      .x_label("observation")
      .y_label("$\\sigma_t$")
      .caption("$\\gamma < 0$ leverage enabled");
  for (std::int64_t i = 0; i < assets_count; ++i) {
    flash::lineplot(ng_vol, t, asset_column(ngarch.sigma, i))
        .label(std::string(assets.at(i)))
        .width(1.3F);
  }

  auto& ng_dist = studio.pane2("NGARCH Return Distribution");
  ng_dist.place(0, 2)
      .grid(pl::GridStyle::Major)
      .legend(pl::KeyPlace::Auto)
      .legend_frame(pl::KeyFrame::Framed)
      .x_label("$y_t$")
      .y_label("count")
      .caption("Asset 1: histogram + KDE");
  const auto ng_asset1 = asset_column(ngarch.returns, 0);
  constexpr std::int64_t bins = 26;
  flash::histplot(ng_dist, ng_asset1, static_cast<int>(bins)).label("hist");
  const auto [kde_x_raw, kde_y_raw] = flash::detail::gaussian_kde(ng_asset1, 160);
  const auto kde_x = pluss::make_series(kde_x_raw, {static_cast<std::int64_t>(kde_x_raw.size())});
  const auto kde_density = pluss::make_series(kde_y_raw, {static_cast<std::int64_t>(kde_y_raw.size())});
  const double min_value = scalar_f64(ng_asset1.min());
  const double max_value = scalar_f64(ng_asset1.max());
  const double bin_width = std::max((max_value - min_value) / static_cast<double>(bins), 1.0e-6);
  const auto kde_counts = kde_density * (static_cast<double>(ng_asset1.numel()) * bin_width);
  flash::kdeplot(ng_dist, kde_x, kde_counts).label("kde");

  auto& g_ret = studio.pane2(R"(GARCH Returns ($\gamma = 0$))");
  g_ret.place(1, 0)
      .grid(pl::GridStyle::Major)
      .legend(pl::KeyPlace::Auto)
      .legend_frame(pl::KeyFrame::Soft)
      .x_label("observation")
      .y_label("$y_t$")
      .caption("$\\omega, \\alpha, \\beta, \\lambda$ shared");
  for (std::int64_t i = 0; i < assets_count; ++i) {
    flash::lineplot(g_ret, t, asset_column(garch.returns, i))
        .label(std::string(assets.at(i)))
        .width(1.35F);
  }

  auto& g_vol = studio.pane2(R"(GARCH Volatility ($\gamma = 0$))");
  g_vol.place(1, 1)
      .grid(pl::GridStyle::Major)
      .legend(pl::KeyPlace::Auto)
      .legend_frame(pl::KeyFrame::None)
      .x_label("observation")
      .y_label(R"($\sigma_t$)")
      .caption("no leverage channel");
  for (std::int64_t i = 0; i < assets_count; ++i) {
    flash::lineplot(g_vol, t, asset_column(garch.sigma, i))
        .label(std::string(assets.at(i)))
        .width(1.3F);
  }

  auto& compare = studio.pane2("Leverage Impact: RMS Delta Sigma");
  compare.place(1, 2)
      .grid(pl::GridStyle::Major)
      .legend(pl::KeyPlace::Auto)
      .legend_frame(pl::KeyFrame::Soft)
      .x_label("observation")
      .y_label(R"($\mathrm{RMS}\,\Delta\sigma_t$)")
      .caption("cross-asset volatility divergence");
  flash::lineplot(compare, t, spread_rms).label("NGARCH - GARCH RMS").width(1.9F);

  studio.save_svg(out_path("plusspl_ngarch_demo.svg"));
  studio.save_pdf(out_path("plusspl_ngarch_demo.pdf"));
  try {
    studio.save_png(out_path("plusspl_ngarch_demo.png"), 170);
  } catch (const std::exception&) {
    // PNG export depends on an external rasterizer (rsvg-convert/inkscape/magick).
  }
  studio.show();
  return 0;
}
