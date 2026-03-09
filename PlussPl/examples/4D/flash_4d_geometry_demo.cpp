#include <pluss/pluss.h>

#include <array>
#include <bit>
#include <cmath>
#include <filesystem>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;

} // namespace

int main() {
  using namespace pluss::pl;
  using namespace pluss::pl::flash;

  const auto output_dir = std::filesystem::path(PLUSSPL_EXAMPLES_SOURCE_DIR) / "outputs" / "4D";
  std::filesystem::create_directories(output_dir);
  const auto out_path = [&output_dir](std::string_view name) {
    return (output_dir / std::string(name)).string();
  };

  Studio fig("flash_4d_geometry_demo", Theme::paper());
  set_context(fig, 1520.0, 980.0, 1.0, 1.0);
  fig.mosaic(2, 2, 18.0, 16.0);

  constexpr std::int64_t n = 560;
  std::vector<double> hx;
  std::vector<double> hy;
  std::vector<double> hz;
  std::vector<double> hw;
  hx.reserve(static_cast<std::size_t>(n));
  hy.reserve(static_cast<std::size_t>(n));
  hz.reserve(static_cast<std::size_t>(n));
  hw.reserve(static_cast<std::size_t>(n));
  for (const auto i : std::views::iota(std::int64_t{0}, n)) {
    const double t = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(n - 1);
    hx.push_back(std::cos(t));
    hy.push_back(std::sin(t));
    hz.push_back(0.72 * std::cos(2.0 * t));
    hw.push_back(0.60 * std::sin(3.0 * t));
  }

  HyperPlotOptions path_opts;
  path_opts.projection = HyperProjection::PerspectiveW;
  path_opts.rot_xw_deg = 30.0;
  path_opts.rot_yz_deg = 15.0;
  path_opts.rot_zw_deg = 22.0;
  path_opts.w_perspective_distance = 3.4;

  auto& path_pane = fig.pane3("parametric path in R4");
  path_pane.place(0, 0).grid(GridStyle::Major).x_label("x").y_label("y").z_label("z");
  auto& path_trace = path4(path_pane, hx, hy, hz, hw, path_opts).label("projected path4");
  trace_preset3d(path_trace, TracePreset3D::Path);

  HyperPlotOptions cloud_opts;
  cloud_opts.projection = HyperProjection::PerspectiveW;
  cloud_opts.rot_xy_deg = 12.0;
  cloud_opts.rot_xw_deg = 26.0;
  cloud_opts.rot_yw_deg = 20.0;
  cloud_opts.rot_zw_deg = 35.0;
  cloud_opts.w_perspective_distance = 3.8;
  cloud_opts.w_slice = Range1d{-0.55, 0.55};
  cloud_opts.point_size = 14.0;

  auto& cloud_pane = fig.pane3("w-slice cloud in R4");
  cloud_pane.place(0, 1).grid(GridStyle::Major).x_label("x").y_label("y").z_label("z");
  auto& cloud_trace = cloud4(cloud_pane, hx, hy, hz, hw, cloud_opts).label("projected cloud4");
  trace_preset3d(cloud_trace, TracePreset3D::Cloud);

  std::vector<std::array<double, 4>> tesseract_vertices;
  tesseract_vertices.reserve(16);
  for (const auto id : std::views::iota(0, 16)) {
    const auto sign = [id](int bit) { return ((id >> bit) & 1) == 0 ? -1.0 : 1.0; };
    tesseract_vertices.push_back({sign(0), sign(1), sign(2), sign(3)});
  }

  std::vector<double> tx;
  std::vector<double> ty;
  std::vector<double> tz;
  std::vector<double> tw;
  tx.reserve(tesseract_vertices.size());
  ty.reserve(tesseract_vertices.size());
  tz.reserve(tesseract_vertices.size());
  tw.reserve(tesseract_vertices.size());
  for (const auto& v : tesseract_vertices) {
    tx.push_back(v[0]);
    ty.push_back(v[1]);
    tz.push_back(v[2]);
    tw.push_back(v[3]);
  }

  HyperPlotOptions tesseract_opts;
  tesseract_opts.projection = HyperProjection::PerspectiveW;
  tesseract_opts.rot_xy_deg = 12.0;
  tesseract_opts.rot_xw_deg = 34.0;
  tesseract_opts.rot_yz_deg = 18.0;
  tesseract_opts.rot_zw_deg = 26.0;
  tesseract_opts.w_perspective_distance = 4.0;
  tesseract_opts.point_size = 18.0;

  auto& tesseract_pane = fig.pane3("tesseract edges (R4 -> R3)");
  tesseract_pane.place(1, 0).grid(GridStyle::Major).x_label("x").y_label("y").z_label("z");

  auto& tesseract_cloud = cloud4(tesseract_pane, tx, ty, tz, tw, tesseract_opts).label("vertices");
  trace_preset3d(tesseract_cloud, TracePreset3D::Cloud);

  for (const auto a : std::views::iota(0, 16)) {
    for (const auto b : std::views::iota(a + 1, 16)) {
      const int diff_bits = std::popcount(static_cast<unsigned>(a ^ b));
      if (diff_bits != 1) {
        continue;
      }
      const std::array<double, 2> ex{tesseract_vertices[static_cast<std::size_t>(a)][0],
                                     tesseract_vertices[static_cast<std::size_t>(b)][0]};
      const std::array<double, 2> ey{tesseract_vertices[static_cast<std::size_t>(a)][1],
                                     tesseract_vertices[static_cast<std::size_t>(b)][1]};
      const std::array<double, 2> ez{tesseract_vertices[static_cast<std::size_t>(a)][2],
                                     tesseract_vertices[static_cast<std::size_t>(b)][2]};
      const std::array<double, 2> ew{tesseract_vertices[static_cast<std::size_t>(a)][3],
                                     tesseract_vertices[static_cast<std::size_t>(b)][3]};
      auto& edge = path4(tesseract_pane, ex, ey, ez, ew, tesseract_opts);
      edge.width(1.0F);
    }
  }

  std::vector<double> mx;
  std::vector<double> my;
  std::vector<double> mz;
  std::vector<double> mw;
  constexpr std::int64_t u_steps = 42;
  constexpr std::int64_t v_steps = 56;
  mx.reserve(static_cast<std::size_t>(u_steps * v_steps));
  my.reserve(static_cast<std::size_t>(u_steps * v_steps));
  mz.reserve(static_cast<std::size_t>(u_steps * v_steps));
  mw.reserve(static_cast<std::size_t>(u_steps * v_steps));
  for (const auto iu : std::views::iota(std::int64_t{0}, u_steps)) {
    const double u = 0.15 + (kPi - 0.30) * static_cast<double>(iu) / static_cast<double>(u_steps - 1);
    for (const auto iv : std::views::iota(std::int64_t{0}, v_steps)) {
      const double v = 2.0 * kPi * static_cast<double>(iv) / static_cast<double>(v_steps - 1);
      mx.push_back(std::cos(u));
      my.push_back(std::sin(u) * std::cos(v));
      mz.push_back(std::sin(u) * std::sin(v) * std::cos(0.5 * u));
      mw.push_back(0.55 * std::sin(u) * std::sin(v) * std::sin(0.5 * u));
    }
  }

  HyperPlotOptions manifold_opts;
  manifold_opts.projection = HyperProjection::PerspectiveW;
  manifold_opts.rot_xy_deg = 18.0;
  manifold_opts.rot_xw_deg = 24.0;
  manifold_opts.rot_yz_deg = 14.0;
  manifold_opts.rot_yw_deg = 22.0;
  manifold_opts.rot_zw_deg = 16.0;
  manifold_opts.w_perspective_distance = 3.5;
  manifold_opts.point_size = 6.0;

  auto& manifold_pane = fig.pane3("manifold sample (R4 -> R3)");
  manifold_pane.place(1, 1).grid(GridStyle::Major).x_label("x").y_label("y").z_label("z");
  auto& manifold_cloud = cloud4(manifold_pane, mx, my, mz, mw, manifold_opts).label("projected manifold");
  trace_preset3d(manifold_cloud, TracePreset3D::Cloud);

  fig.headline("Geometric 4D projection rendering: path, slice, tesseract, manifold");
  fig.save_pdf(out_path("plusspl_flash_4d_geometry_demo.pdf"));
  return 0;
}
