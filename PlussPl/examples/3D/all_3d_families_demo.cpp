#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <span>
#include <tuple>
#include <vector>

namespace {

pluss::Series peaks_like(std::int64_t rows, std::int64_t cols) {
  const auto x_axis = pluss::linspace(-3.0, 3.0, cols, pluss::ScalarType::Float64).reshape({1, cols});
  const auto y_axis = pluss::linspace(-3.0, 3.0, rows, pluss::ScalarType::Float64).reshape({rows, 1});
  const std::array<std::int64_t, 2> x_tile{rows, 1};
  const std::array<std::int64_t, 2> y_tile{1, cols};
  const auto x = x_axis.tile(std::span<const std::int64_t>(x_tile.data(), x_tile.size()));
  const auto y = y_axis.tile(std::span<const std::int64_t>(y_tile.data(), y_tile.size()));

  const auto crest = (1.0 - x).pow(2.0).mul(3.0).mul(x.pow(2.0).neg().sub(y.add(1.0).pow(2.0)).exp());
  const auto saddle = x.div(5.0).sub(x.pow(3.0)).sub(y.pow(5.0)).mul(-10.0).mul(x.pow(2.0).add(y.pow(2.0)).neg().exp());
  const auto tail = x.add(1.0).pow(2.0).neg().sub(y.pow(2.0)).exp().mul(-1.0 / 3.0);
  return crest.add(saddle).add(tail);
}

std::tuple<pluss::Series, pluss::Series, pluss::Series, pluss::Series> synthetic_volume(
    std::int64_t nx,
    std::int64_t ny,
    std::int64_t nz) {
  auto x = pluss::linspace(-1.4, 1.4, nx, pluss::ScalarType::Float64);
  auto y = pluss::linspace(-1.4, 1.4, ny, pluss::ScalarType::Float64);
  auto z = pluss::linspace(-1.2, 1.2, nz, pluss::ScalarType::Float64);
  const auto xv = x.to_vec<double>();
  const auto yv = y.to_vec<double>();
  const auto zv = z.to_vec<double>();

  std::vector<double> values;
  values.reserve(static_cast<std::size_t>(nx * ny * nz));
  for (std::int64_t ix = 0; ix < nx; ++ix) {
    for (std::int64_t iy = 0; iy < ny; ++iy) {
      for (std::int64_t iz = 0; iz < nz; ++iz) {
        const double px = xv[static_cast<std::size_t>(ix)];
        const double py = yv[static_cast<std::size_t>(iy)];
        const double pz = zv[static_cast<std::size_t>(iz)];
        const double sphere = std::exp(-(px * px + py * py + pz * pz) * 2.6);
        const double lobe = 0.55 * std::exp(-((px - 0.55) * (px - 0.55) + (py + 0.35) * (py + 0.35) + (pz - 0.15) * (pz - 0.15)) * 8.5);
        const double shear = 0.15 * std::sin(3.2 * px + 2.1 * py) * std::cos(2.4 * pz);
        values.push_back(sphere + lobe + shear);
      }
    }
  }

  return {x, y, z, pluss::make_series(values, {nx, ny, nz})};
}

} // namespace

int main() {
  namespace pl = pluss::pl;
  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
  std::filesystem::create_directories(root / "outputs" / "svg");
  std::filesystem::create_directories(root / "outputs" / "pdf");
  std::filesystem::create_directories(root / "outputs" / "pdfs");

  pl::Studio gallery("plusspl-all-3d-families", pl::Theme::ocean());
  gallery.size(1560.0, 1040.0);
  gallery.mosaic(2, 3, 20.0, 18.0);
  gallery.headline("PlussPl All 3D Families");
  gallery.legend(pl::KeyDock::East).z_caption("terrain-group", "shared z scale");

  const auto xs = pluss::linspace(-3.0, 3.0, 34, pluss::ScalarType::Float64);
  const auto ys = pluss::linspace(-3.0, 3.0, 34, pluss::ScalarType::Float64);
  const auto zz = peaks_like(34, 34);

  auto& pane1 = gallery.pane3("sheet + lattice");
  pane1.place(0, 0).grid(pl::GridStyle::Major).caption("surface and wireframe families").align_z("terrain-group");
  pane1.lights().sun_key();
  pane1.sheet(xs, ys, zz)
      .label("sheet")
      .shade(pl::ShadeMode::Smooth)
      .lighting(pl::LightingMode::Gouraud)
      .material(pl::MaterialProfile::Shiny)
      .palette(pl::PaletteRamp::lagoon());
  pane1.lattice(xs, ys, zz, zz).label("lattice").width(0.9F);

  auto& pane2 = gallery.pane3("ridge3");
  pane2.place(0, 1).grid(pl::GridStyle::Major).caption("elevated contour family").align_z("terrain-group");
  pane2.lens().look(-44.0, 25.0);
  pane2.ridge3(xs, ys, zz, std::vector<double>{-5.0, -3.0, -1.0, 0.5, 2.0, 4.0, 6.0})
      .label("ridge3")
      .palette(pl::PaletteRamp::thermal())
      .annotate_levels();

  auto& pane3 = gallery.pane3("path3 + cloud3");
  pane3.place(0, 2).grid(pl::GridStyle::Major).caption("trajectory and point-cloud families");
  pane3.lights().headlight();
  const auto t = pluss::linspace(0.0, 10.0, 240, pluss::ScalarType::Float64);
  const auto px = t.mul(1.9).cos();
  const auto py = t.mul(1.9).sin();
  const auto pz = t.mul(0.22);
  pane3.path3(px, py, pz).label("path3").width(2.1F);
  const auto host_px = px.to_vec<double>();
  const auto host_py = py.to_vec<double>();
  const auto host_pz = pz.to_vec<double>();
  std::vector<double> sample_x;
  std::vector<double> sample_y;
  std::vector<double> sample_z;
  std::vector<double> sample_size;
  sample_x.reserve(host_px.size() / 6);
  sample_y.reserve(host_py.size() / 6);
  sample_z.reserve(host_pz.size() / 6);
  sample_size.reserve(host_pz.size() / 6);
  for (std::size_t i = 0; i < host_px.size(); i += 6) {
    sample_x.push_back(host_px[i]);
    sample_y.push_back(host_py[i]);
    sample_z.push_back(host_pz[i]);
    sample_size.push_back((host_pz[i] + 1.0) * 26.0);
  }
  pane3.cloud3(sample_x,
               sample_y,
               sample_z,
               sample_size,
               sample_z)
      .label("cloud3");

  auto& pane4 = gallery.pane3("spire3");
  pane4.place(1, 0).grid(pl::GridStyle::Major).caption("3D bar family").align_z("terrain-group");
  const auto spire = pluss::make_series(
      std::vector<double>{
          0.4, 0.8, 1.2, 0.9,
          0.7, 1.1, 1.6, 1.3,
          0.9, 1.4, 2.0, 1.7,
          0.6, 1.0, 1.5, 1.2,
      },
      {4, 4});
  pane4.spire3(std::array<double, 4>{1, 2, 3, 4},
               std::array<double, 4>{1, 2, 3, 4},
               spire)
      .label("spire3")
      .palette(pl::PaletteRamp::meadow())
      .shade(pl::ShadeMode::Faceted);

  auto& pane5 = gallery.pane3("isoshell");
  pane5.place(1, 1).grid(pl::GridStyle::Major).caption("volumetric isosurface family");
  pane5.lens().look(-32.0, 20.0);
  pane5.lights().sun_key();
  auto [vx, vy, vz, field] = synthetic_volume(24, 24, 22);
  pane5.isoshell(vx, vy, vz, field, std::vector<double>{0.28, 0.44, 0.62})
      .label("isoshell")
      .palette(pl::PaletteRamp::thermal())
      .shade(pl::ShadeMode::Smooth)
      .lighting(pl::LightingMode::Gouraud)
      .transparent();

  auto& pane6 = gallery.pane3("volume cues");
  pane6.place(1, 2).grid(pl::GridStyle::Major).caption("annotation and alignment controls");
  pane6.lens().look(-50.0, 28.0);
  pane6.path3(std::array<double, 5>{0.0, 0.2, 0.5, 0.8, 1.0},
              std::array<double, 5>{0.0, 0.5, 0.9, 0.4, 0.1},
              std::array<double, 5>{0.2, 0.5, 0.9, 1.3, 1.7})
      .label("cued path")
      .stroke(pl::Rgba::from_u8(35, 80, 140));
  pane6.rule_z(1.0, "z-rule");
  pane6.band_z({0.55, 1.25}, "z-band");
  pane6.pin("anchor", pl::Vec3d{0.5, 0.9, 0.9});

  gallery.save_svg((root / "outputs" / "svg" / "plusspl_all_3d_families.svg").string());
  gallery.save_pdf((root / "outputs" / "pdf" / "plusspl_all_3d_families.pdf").string());
  gallery.save_pdf((root / "outputs" / "pdfs" / "plusspl_all_3d_families.pdf").string());
  return 0;
}
