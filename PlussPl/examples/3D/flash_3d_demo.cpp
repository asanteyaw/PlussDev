#include <pluss/pluss.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

int main() {
  using namespace pluss::pl;
  using namespace pluss::pl::flash;

  const auto output_dir = std::filesystem::path(PLUSSPL_EXAMPLES_SOURCE_DIR) / "outputs" / "3D";
  std::filesystem::create_directories(output_dir);
  const auto out_path = [&output_dir](std::string_view name) {
    return (output_dir / std::string(name)).string();
  };

  Studio fig("flash_3d_demo", Theme::paper());
  set_context(fig, 1400.0, 900.0, 1.05, 1.05);
  fig.mosaic(2, 2, 16.0, 16.0);

  auto& pane_a = fig.pane3("Path + Cloud");
  pane_a.place(0, 0).grid(GridStyle::Major).x_label("x").y_label("y").z_label("z");
  scene3d(
      pane_a,
      Scene3DOptions{
          .preset = ScenePreset3D::Survey,
      });
  auto& trace_path = path3(pane_a,
        std::array<double, 6>{0.0, 0.4, 0.8, 1.2, 1.6, 2.0},
        std::array<double, 6>{0.0, 0.3, 0.9, 1.1, 1.4, 2.0},
        std::array<double, 6>{0.2, 0.5, 0.7, 1.1, 1.35, 1.8})
      .label("flash::path3");
  trace_preset3d(trace_path, TracePreset3D::Path);
  auto& trace_cloud = cloud3(pane_a,
         std::array<double, 6>{0.2, 0.5, 0.9, 1.1, 1.5, 1.9},
         std::array<double, 6>{0.1, 0.4, 0.85, 1.0, 1.35, 1.8},
         std::array<double, 6>{0.3, 0.7, 0.9, 1.25, 1.52, 1.96},
         std::array<double, 6>{10.0, 12.0, 14.0, 16.0, 18.0, 20.0},
         std::array<double, 6>{0.1, 0.22, 0.4, 0.58, 0.73, 0.9})
      .label("flash::cloud3");
  trace_preset3d(trace_cloud, TracePreset3D::Cloud);
  plot3D(pane_a,
         std::array<double, 4>{0.0, 0.7, 1.4, 2.1},
         std::array<double, 4>{0.0, 0.5, 1.1, 1.9},
         std::array<double, 4>{0.15, 0.62, 1.05, 1.7})
      .label("plot3D");
  scatter3D(pane_a,
            std::array<double, 4>{0.25, 0.8, 1.55, 1.95},
            std::array<double, 4>{0.15, 0.55, 1.25, 1.75},
            std::array<double, 4>{0.28, 0.75, 1.2, 1.85})
      .label("scatter3D");

  auto z_surface = pluss::make_series(
      std::vector<double>{
          0.3, 0.55, 0.8, 1.0,
          0.45, 0.72, 1.02, 1.28,
          0.62, 0.95, 1.26, 1.5,
      },
      {3, 4});
  auto c_surface = pluss::make_series(
      std::vector<double>{
          0.1, 0.25, 0.42, 0.6,
          0.2, 0.38, 0.56, 0.72,
          0.34, 0.5, 0.68, 0.85,
      },
      {3, 4});

  auto& pane_b = fig.pane3("Surface + Lattice");
  pane_b.place(0, 1).grid(GridStyle::Major).x_label("x").y_label("y").z_label("z");
  scene3d(
      pane_b,
      Scene3DOptions{
          .preset = ScenePreset3D::Presentation,
      });
  auto& trace_sheet = sheet(
      pane_b,
      std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
      std::array<double, 3>{0.0, 1.0, 2.0},
      z_surface,
      c_surface)
                          .label("flash::sheet");
  trace_preset3d(trace_sheet, TracePreset3D::Surface);
  auto& trace_lattice =
      lattice(pane_b, std::array<double, 4>{0.0, 1.0, 2.0, 3.0}, std::array<double, 3>{0.0, 1.0, 2.0}, z_surface)
          .label("flash::lattice");
  trace_preset3d(trace_lattice, TracePreset3D::Wireframe);
  plot_surface(pane_b, std::array<double, 4>{0.0, 1.0, 2.0, 3.0}, std::array<double, 3>{0.0, 1.0, 2.0}, z_surface)
      .label("plot_surface");
  plot_wireframe(pane_b, std::array<double, 4>{0.0, 1.0, 2.0, 3.0}, std::array<double, 3>{0.0, 1.0, 2.0}, z_surface)
      .label("plot_wireframe");

  auto& pane_c = fig.pane3("Ridge + Spire");
  pane_c.place(1, 0).grid(GridStyle::Major).x_label("x").y_label("y").z_label("z");
  scene3d(
      pane_c,
      Scene3DOptions{
          .preset = ScenePreset3D::Studio,
      });
  ridge3(pane_c,
         std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
         std::array<double, 3>{0.0, 1.0, 2.0},
         z_surface,
         6)
      .label("flash::ridge3");
  spire3(pane_c, std::array<double, 4>{0.0, 1.0, 2.0, 3.0}, std::array<double, 3>{0.0, 1.0, 2.0}, z_surface)
      .label("flash::spire3");
  contour3D(pane_c,
            std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
            std::array<double, 3>{0.0, 1.0, 2.0},
            z_surface,
            6)
      .label("contour3D");
  bar3d(pane_c,
        std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
        std::array<double, 3>{0.0, 1.0, 2.0},
        z_surface)
      .label("bar3d");

  auto iso = pluss::make_series(
      std::vector<double>{
          0.0, 0.1, 0.2, 0.12, 0.4, 0.75, 0.25, 0.72, 1.05,
          0.1, 0.22, 0.35, 0.24, 0.65, 1.02, 0.41, 0.98, 1.36,
          0.2, 0.33, 0.48, 0.36, 0.82, 1.2, 0.57, 1.18, 1.62,
      },
      {3, 3, 3});
  auto& pane_d = fig.pane3("IsoShell");
  pane_d.place(1, 1).grid(GridStyle::Major).x_label("x").y_label("y").z_label("z");
  scene3d(
      pane_d,
      Scene3DOptions{
          .preset = ScenePreset3D::Topology,
          .camera = Camera3DOptions{
              .azimuth_deg = 52.0,
              .elevation_deg = 34.0,
              .view_angle_deg = 26.0,
              .projection = ProjectionKind::Perspective,
          },
          .lighting = Lighting3DOptions{
              .clear_first = true,
              .headlight = true,
              .lamp_left = true,
              .lamp_right = true,
              .sun_key = false,
          },
      });
  auto& trace_iso = isoshell(pane_d,
           std::array<double, 3>{0.0, 1.0, 2.0},
           std::array<double, 3>{0.0, 1.0, 2.0},
           std::array<double, 3>{0.0, 1.0, 2.0},
           iso,
           4)
      .label("flash::isoshell");
  trace_preset3d(trace_iso, TracePreset3D::Volume);

  // fig.save_svg(out_path("plusspl_flash_3d_demo.svg"));
  fig.save_pdf(out_path("plusspl_flash_3d_demo.pdf"));

  Studio camera_gallery("flash_3d_camera_gallery", Theme::paper());
  set_context(camera_gallery, 1500.0, 520.0, 1.0, 1.0);
  camera_gallery.mosaic(1, 4, 10.0, 8.0);
  const std::array<CameraPreset3D, 4> presets{
      CameraPreset3D::TopDown,
      CameraPreset3D::Front,
      CameraPreset3D::Side,
      CameraPreset3D::Isometric,
  };
  const std::array<std::string_view, 4> preset_names{
      "top-down",
      "front",
      "side",
      "isometric",
  };
  for (std::size_t i = 0; i < presets.size(); ++i) {
    auto& pane = camera_gallery.pane3(std::string(preset_names[i]));
    pane.place(0, static_cast<std::int64_t>(i)).grid(GridStyle::Major).x_label("x").y_label("y").z_label("z");
    camera_preset3d(pane, presets[i]);
    lighting3d(pane, Lighting3DOptions{.clear_first = true, .headlight = true});
    sheet(pane,
          std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
          std::array<double, 3>{0.0, 1.0, 2.0},
          z_surface)
        .label("surface");
  }
  // camera_gallery.save_svg(out_path("plusspl_flash_3d_camera_gallery.svg"));
  camera_gallery.save_pdf(out_path("plusspl_flash_3d_camera_gallery.pdf"));

  Studio scene_gallery("flash_3d_scene_gallery", Theme::paper());
  set_context(scene_gallery, 1420.0, 980.0, 1.0, 1.0);
  scene_gallery.mosaic(2, 2, 14.0, 12.0);
  const std::array<ScenePreset3D, 4> scene_presets{
      ScenePreset3D::Survey,
      ScenePreset3D::Presentation,
      ScenePreset3D::Studio,
      ScenePreset3D::Topology,
  };
  const std::array<std::string_view, 4> scene_names{
      "survey",
      "presentation",
      "studio",
      "topology",
  };
  for (std::size_t i = 0; i < scene_presets.size(); ++i) {
    auto& pane = scene_gallery.pane3(std::string(scene_names[i]));
    const auto row = static_cast<std::int64_t>(i / 2);
    const auto col = static_cast<std::int64_t>(i % 2);
    pane.place(row, col).x_label("x").y_label("y").z_label("z");
    scene_preset3d(pane, scene_presets[i]);
    auto& trace = sheet(
        pane,
        std::array<double, 4>{0.0, 1.0, 2.0, 3.0},
        std::array<double, 3>{0.0, 1.0, 2.0},
        z_surface)
                      .label("surface");
    trace_preset3d(trace, TracePreset3D::Surface);
  }
  // scene_gallery.save_svg(out_path("plusspl_flash_3d_scene_gallery.svg"));
  scene_gallery.save_pdf(out_path("plusspl_flash_3d_scene_gallery.pdf"));

  constexpr std::int64_t trio_cases = 240;
  std::vector<double> trio_flat;
  trio_flat.reserve(static_cast<std::size_t>(trio_cases * 5));
  for (const auto i : std::views::iota(std::int64_t{0}, trio_cases)) {
    const double t = static_cast<double>(i) / 18.0;
    const double f1 = std::sin(t) + 0.12 * std::cos(2.0 * t);
    const double f2 = std::cos(0.7 * t) + 0.1 * std::sin(1.6 * t);
    const double f3 = 0.35 * f1 + 0.65 * f2;
    const double f4 = 0.06 * static_cast<double>(i) + 0.25 * std::sin(0.5 * t);
    const double f5 = f1 - f2;
    trio_flat.push_back(f1);
    trio_flat.push_back(f2);
    trio_flat.push_back(f3);
    trio_flat.push_back(f4);
    trio_flat.push_back(f5);
  }
  const auto trio_matrix = pluss::make_series(std::move(trio_flat), {trio_cases, 5});
  const std::vector<std::string> trio_names{"trend", "cycle", "blend", "drift", "spread"};

  Studio trio_matrix_fig("flash_triplot_matrix_demo", Theme::paper());
  TriplotOptions trio_matrix_opts;
  trio_matrix_opts.kind = TriplotKind::Cloud;
  trio_matrix_opts.rows = 2;
  trio_matrix_opts.cols = 3;
  trio_matrix_opts.max_panels = 6;
  trio_matrix_opts.max_points = 180;
  triplot(trio_matrix_fig, trio_matrix, trio_names, Axis::Cols, trio_matrix_opts);
  trio_matrix_fig.headline("triplot (matrix)");
  trio_matrix_fig.save_pdf(out_path("plusspl_flash_triplot_matrix_demo.pdf"));

  Studio trio_dataset_fig("flash_triplot_dataset_demo", Theme::paper());
  TriplotOptions trio_dataset_opts;
  trio_dataset_opts.kind = TriplotKind::Path;
  trio_dataset_opts.rows = 1;
  trio_dataset_opts.cols = 3;
  trio_dataset_opts.max_panels = 3;
  trio_dataset_opts.max_points = 140;
  trio_dataset_opts.feature_names = {"trend", "blend", "drift", "spread"};
  const auto trio_data = make_dataset(trio_matrix, trio_names);
  triplot(trio_dataset_fig, trio_data, trio_dataset_opts);
  trio_dataset_fig.headline("triplot (dataset subset)");
  trio_dataset_fig.save_pdf(out_path("plusspl_flash_triplot_dataset_demo.pdf"));

  fig.show("PlussPl Flash 3D", 1400, 900, true);
  return 0;
}
