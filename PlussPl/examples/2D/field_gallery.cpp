#include <pluss/pluss.h>

#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

namespace {

pluss::Series peaks_like(std::int64_t rows, std::int64_t cols) {
  std::vector<double> values;
  values.reserve(static_cast<std::size_t>(rows * cols));
  for (std::int64_t i = 0; i < rows; ++i) {
    const double y = -3.0 + 6.0 * static_cast<double>(i) / static_cast<double>(rows - 1);
    for (std::int64_t j = 0; j < cols; ++j) {
      const double x = -3.0 + 6.0 * static_cast<double>(j) / static_cast<double>(cols - 1);
      const double z = 3.0 * std::pow(1.0 - x, 2.0) * std::exp(-(x * x) - std::pow(y + 1.0, 2.0))
                     - 10.0 * (x / 5.0 - std::pow(x, 3.0) - std::pow(y, 5.0)) * std::exp(-(x * x) - (y * y))
                     - (1.0 / 3.0) * std::exp(-std::pow(x + 1.0, 2.0) - (y * y));
      values.push_back(z);
    }
  }
  return pluss::make_series(values, {rows, cols});
}

} // namespace

int main() {
  namespace pl = pluss::pl;
  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
  std::filesystem::create_directories(root / "outputs" / "svg");
  std::filesystem::create_directories(root / "outputs" / "pdf");
  std::filesystem::create_directories(root / "outputs" / "pdfs");

  pl::Studio gallery("plusspl-2d", pl::Theme::ocean());
  gallery.size(1380.0, 1720.0);
  gallery.mosaic(4, 2, 22.0, 18.0);
  gallery.headline("Pluss 2D Field Gallery");
  gallery.trace_key(pl::KeyDock::East)
      .x_caption("signal-x", "shared frequency axis")
      .y_caption("signal-y", "shared response bands");

  auto& sweep = gallery.pane2("Signal sweep");
  sweep.grid(pl::GridStyle::MajorMinor)
       .scale_x(pl::ScalePolicy::log10())
       .caption("log sweep with acceptance window")
       .place(0, 0)
       .align_x("signal-x")
       .align_y("signal-y")
       .rule_x(100.0, "handoff")
       .band_y({0.55, 1.08}, "target band");

  sweep.trend(std::vector<double>{1.0, 3.0, 10.0, 30.0, 100.0, 300.0, 1000.0},
              std::vector<double>{0.18, 0.32, 0.58, 0.91, 1.22, 1.05, 0.76})
      .label("system A")
      .width(2.3F);

  sweep.trend(std::vector<double>{1.0, 3.0, 10.0, 30.0, 100.0, 300.0, 1000.0},
              std::vector<double>{0.08, 0.24, 0.47, 0.84, 1.05, 0.95, 0.62})
      .label("system B")
      .width(2.0F)
      .stroke(pl::Rgba::from_u8(196, 75, 59));
  sweep.pin("peak gain", 100.0, 1.22);

  auto& halo = gallery.pane2("Polar envelope");
  halo.grid(pl::GridStyle::Major).caption("beam shape with broadside emphasis").place(0, 1);
  auto theta = pluss::linspace(0.0, 2.0 * 3.14159265358979323846, 241, pluss::ScalarType::Float64);
  auto rho = theta.mul(4.0).sin().mul(0.25).add(theta.mul(9.0).cos().mul(0.1)).add(0.8);
  halo.radial(theta, rho).label("beam pattern").width(2.0F);

  auto& terrain = gallery.pane2("Contour field");
  terrain.grid(pl::GridStyle::Major)
         .caption("isobands, ridges, and annotations")
         .place(1, 0)
         .scale_guide()
         .align_x("terrain-x")
         .align_y("terrain-y")
         .rule_y(0.0, "equator")
         .band_x({-1.25, 0.95}, "focus slice")
         .color_limits({-6.0, 8.0});
  gallery.x_caption("terrain-x", "shared x field axis");
  gallery.y_caption("terrain-y", "shared y field axis");

  auto gx = pluss::linspace(-3.0, 3.0, 45, pluss::ScalarType::Float64);
  auto gy = pluss::linspace(-3.0, 3.0, 45, pluss::ScalarType::Float64);
  const auto bands = std::vector<double>{-6.0, -3.0, -1.0, 0.0, 1.5, 3.0, 5.5, 8.0};

  terrain.isoband(gx, gy, peaks_like(45, 45), bands)
      .label("isobands")
      .palette(pl::PaletteRamp::thermal());

  terrain.contour(gx, gy, peaks_like(45, 45), bands)
      .label("iso-ridges")
      .width(1.2F)
      .palette(pl::PaletteRamp::thermal())
      .annotate_levels();
  terrain.pin("summit", 0.15, 1.35, 34.0, -22.0);

  auto& marks = gallery.pane2("Mark cloud");
  marks.grid(pl::GridStyle::Major)
       .caption("Series-backed point cloud")
       .place(1, 1)
       .x_label("phase")
       .y_label("response")
       .scale_guide()
       .color_limits({-1.4, 1.4});
  auto mark_x = pluss::linspace(-4.0, 4.0, 64, pluss::ScalarType::Float64);
  auto mark_y = mark_x.sin().add(mark_x.mul(0.7).cos().mul(0.35));
  auto mark_size = mark_y.abs().mul(42.0).add(16.0);
  marks.cloud(mark_x, mark_y, mark_size, mark_y)
      .label("sensor cloud")
      .palette(pl::PaletteRamp::ocean())
      .color_limits({-1.4, 1.4});
  marks.rule_y(0.0, "neutral");

  auto& bucket = gallery.pane2("Bucket profile");
  bucket.grid(pl::GridStyle::Major)
        .caption("Series histc-driven bucket counts")
        .place(2, 0)
        .x_label("value")
        .y_label("count");
  auto sample_axis = pluss::linspace(-2.8, 2.8, 260, pluss::ScalarType::Float64);
  auto sample_values = sample_axis.sin().mul(1.15).add(sample_axis.mul(0.5).cos().mul(0.55));
  bucket.bucket(sample_values, 18).label("bucket count").palette(pl::PaletteRamp::meadow());

  auto& spread = gallery.pane2("Quartet + violin spread");
  spread.grid(pl::GridStyle::Major)
        .caption("quartile summary plus KDE violin envelope")
        .place(2, 1)
        .x_label("group")
        .y_label("value");
  auto group = pluss::make_series(std::vector<double>{
      1,1,1,1,1,1,1,1,1,1,1,1,
      2,2,2,2,2,2,2,2,2,2,2,2,
      3,3,3,3,3,3,3,3,3,3,3,3
  }, {36});
  auto spread_values = pluss::make_series(std::vector<double>{
      -0.8,-0.5,-0.4,-0.2,-0.1,0.0,0.1,0.15,0.2,0.35,0.5,1.1,
      0.2,0.35,0.5,0.55,0.7,0.85,1.0,1.1,1.2,1.3,1.45,2.2,
      -1.1,-0.9,-0.7,-0.5,-0.25,0.05,0.2,0.3,0.45,0.65,0.9,1.8
  }, {36});
  spread.quartet(group, spread_values)
      .label("group spread")
      .palette(pl::PaletteRamp::thermal())
      .stroke(pl::Rgba::from_u8(40, 50, 66));
  spread.violin(group, spread_values)
      .label("group density")
      .palette(pl::PaletteRamp::lagoon())
      .stroke(pl::Rgba::from_u8(30, 74, 122));

  auto& canopy = gallery.pane2("Ribbon canopy");
  canopy.grid(pl::GridStyle::Major)
        .caption("stacked profile ribbons from Series cumsum")
        .place(3, 0)
        .x_label("frame")
        .y_label("load");
  auto frame = pluss::linspace(0.0, 11.0, 12, pluss::ScalarType::Float64);
  auto base_a = frame.mul(0.55).sin().add(1.35);
  auto base_b = frame.mul(0.35).cos().mul(0.8).add(1.0);
  auto base_c = frame.mul(0.22).sin().mul(0.55).add(0.65);
  const auto host_a = base_a.to(pluss::ScalarType::Float64).to_vec<double>();
  const auto host_b = base_b.to(pluss::ScalarType::Float64).to_vec<double>();
  const auto host_c = base_c.to(pluss::ScalarType::Float64).to_vec<double>();
  std::vector<double> canopy_values;
  canopy_values.reserve(host_a.size() * 3U);
  for (std::size_t i = 0; i < host_a.size(); ++i) {
    canopy_values.push_back(host_a[i]);
    canopy_values.push_back(host_b[i]);
    canopy_values.push_back(host_c[i]);
  }
  auto canopy_y = pluss::make_series(canopy_values, {static_cast<std::int64_t>(host_a.size()), 3});
  canopy.ribbon(frame, canopy_y, 0.25)
      .label("load stack")
      .palette(pl::PaletteRamp::meadow());

  auto& whisk = gallery.pane2("Whisk spread");
  whisk.grid(pl::GridStyle::Major)
       .caption("asymmetric uncertainty whiskers")
       .place(3, 1)
       .x_label("trial")
       .y_label("yield");
  auto trial = pluss::linspace(1.0, 8.0, 8, pluss::ScalarType::Float64);
  auto center = trial.mul(0.6).sin().mul(0.45).add(1.55);
  auto down = trial.mul(0.45).cos().abs().mul(0.18).add(0.08);
  auto up = trial.mul(0.25).sin().abs().mul(0.22).add(0.1);
  whisk.whisk(trial, center, down, up)
      .label("uncertainty")
      .width(1.4F)
      .stroke(pl::Rgba::from_u8(120, 54, 136));

  gallery.save_svg((root / "outputs" / "svg" / "plusspl_2d_gallery.svg").string());
  gallery.save_pdf((root / "outputs" / "pdf" / "plusspl_2d_gallery.pdf").string());
  gallery.save_pdf((root / "outputs" / "pdfs" / "plusspl_2d_gallery.pdf").string());
  return 0;
}
