#include <pluss/pluss.h>

#include <array>
#include <filesystem>
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
  camera_preset3d(pane_a, CameraPreset3D::Spatial);
  lighting3d(pane_a, Lighting3DOptions{.clear_first = true, .headlight = true, .lamp_left = true, .sun_key = true});
  path3(pane_a,
        std::array<double, 6>{0.0, 0.4, 0.8, 1.2, 1.6, 2.0},
        std::array<double, 6>{0.0, 0.3, 0.9, 1.1, 1.4, 2.0},
        std::array<double, 6>{0.2, 0.5, 0.7, 1.1, 1.35, 1.8})
      .label("flash::path3");
  cloud3(pane_a,
         std::array<double, 6>{0.2, 0.5, 0.9, 1.1, 1.5, 1.9},
         std::array<double, 6>{0.1, 0.4, 0.85, 1.0, 1.35, 1.8},
         std::array<double, 6>{0.3, 0.7, 0.9, 1.25, 1.52, 1.96},
         std::array<double, 6>{10.0, 12.0, 14.0, 16.0, 18.0, 20.0},
         std::array<double, 6>{0.1, 0.22, 0.4, 0.58, 0.73, 0.9})
      .label("flash::cloud3");
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
  camera_preset3d(pane_b, CameraPreset3D::Isometric);
  lighting3d(pane_b, Lighting3DOptions{.clear_first = true, .headlight = true, .lamp_right = true});
  sheet(pane_b, std::array<double, 4>{0.0, 1.0, 2.0, 3.0}, std::array<double, 3>{0.0, 1.0, 2.0}, z_surface, c_surface)
      .label("flash::sheet");
  lattice(pane_b, std::array<double, 4>{0.0, 1.0, 2.0, 3.0}, std::array<double, 3>{0.0, 1.0, 2.0}, z_surface)
      .label("flash::lattice");
  plot_surface(pane_b, std::array<double, 4>{0.0, 1.0, 2.0, 3.0}, std::array<double, 3>{0.0, 1.0, 2.0}, z_surface)
      .label("plot_surface");
  plot_wireframe(pane_b, std::array<double, 4>{0.0, 1.0, 2.0, 3.0}, std::array<double, 3>{0.0, 1.0, 2.0}, z_surface)
      .label("plot_wireframe");

  auto& pane_c = fig.pane3("Ridge + Spire");
  pane_c.place(1, 0).grid(GridStyle::Major).x_label("x").y_label("y").z_label("z");
  camera3d(pane_c, -42.0, 22.0, ProjectionKind::Perspective, 28.0);
  lighting3d(pane_c, Lighting3DOptions{.clear_first = true, .headlight = true, .sun_key = true});
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
  camera3d(pane_d, 52.0, 34.0, ProjectionKind::Perspective, 26.0);
  lighting3d(pane_d, Lighting3DOptions{.clear_first = true, .headlight = true, .lamp_left = true, .lamp_right = true});
  isoshell(pane_d,
           std::array<double, 3>{0.0, 1.0, 2.0},
           std::array<double, 3>{0.0, 1.0, 2.0},
           std::array<double, 3>{0.0, 1.0, 2.0},
           iso,
           4)
      .label("flash::isoshell");

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

  fig.show("PlussPl Flash 3D", 1400, 900, true);
  return 0;
}
