#include <pluss/pluss.h>

#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int main() {
  using namespace pluss::pl;

  Studio book("core-smoke", Theme::ocean());
  book.size(960.0, 720.0);
  book.headline("Core studio");
  book.trace_key(KeyDock::East).x_caption("shared-signal", "shared signal axis");
  Pane& pane2 = book.pane2("signals");
  pane2.grid(GridStyle::MajorMinor)
      .scale_x(ScalePolicy::log10())
      .x_label("frequency")
      .y_label("gain")
      .key(KeyPlace::NorthEast)
      .align_x("shared-signal")
      .caption("signal pane")
      .rule_x(10.0, "pivot")
      .scale_guide();

  Trace& trend = pane2.trend(std::vector<double>{1.0, 10.0, 100.0},
                             std::array<double, 3>{0.5, 0.8, 1.2});
  trend.label("response").width(2.0F);

  Trace& radial = pane2.radial(std::array<double, 3>{0.0, 1.0, 2.0},
                               std::array<double, 3>{1.0, 0.5, 1.5});
  radial.label("radial");
  pane2.cloud(std::array<double, 3>{1.0, 10.0, 100.0},
              std::array<double, 3>{0.6, 0.9, 1.1},
              std::array<double, 3>{18.0, 24.0, 30.0})
      .label("cloud");
  pane2.bucket(std::array<double, 6>{-1.0, -0.3, 0.0, 0.25, 0.9, 1.1}, 4)
      .label("bucket");
  pane2.pin("crest", 100.0, 1.2);

  Pane& pane3 = book.pane3("surface");
  pane3.grid(GridStyle::Major)
      .x_label("x")
      .y_label("y")
      .z_label("z")
      .caption("surface pane")
      .rule_z(1.0, "slice");
  pane3.lens().spin(12.0, 8.0);
  pane3.lights().sun_key();

  auto z = pluss::make_series(std::vector<double>{0.0, 1.0, 1.5, 3.0}, {2, 2});
  auto c = pluss::make_series(std::vector<double>{0.1, 0.4, 0.6, 0.9}, {2, 2});
  Trace& sheet = pane3.sheet(std::array<double, 2>{0.0, 1.0},
                             std::array<double, 2>{0.0, 1.0},
                             z,
                             c);
  sheet.label("sheet");

  pane3.lattice(std::array<double, 2>{0.0, 1.0},
                std::array<double, 2>{0.0, 1.0},
                z);

  assert(book.pane_count() == 2);
  assert(pane2.trace_count() == 4);
  assert(pane3.trace_count() == 2);
  assert(pane3.lights().lights().size() == 2);
  assert(sheet.kind() == TraceKind::Sheet);
  assert(pane2.x_axis().scale.kind == ScaleKind::Log10);
  assert(pane2.x_axis().major_grid);
  assert(pane2.x_axis().minor_grid);
  assert(pane3.lens().azimuth_deg() == -25.5);
  assert(pane3.lens().elevation_deg() == 38.0);

  const std::string svg = book.to_svg();
  const std::string pdf = book.to_pdf();
  assert(svg.find("<svg") != std::string::npos);
  assert(svg.find("<path") != std::string::npos);
  assert(svg.find("Core studio") != std::string::npos);
  assert(svg.find("cloud") != std::string::npos);
  assert(svg.find("bucket") != std::string::npos);
  assert(svg.find("trace key") != std::string::npos);
  assert(svg.find("pivot") != std::string::npos);
  assert(pdf.rfind("%PDF-1.4", 0) == 0);

  const std::filesystem::path out_dir = std::filesystem::temp_directory_path() / "plusspl_core_smoke";
  std::filesystem::create_directories(out_dir);
  const std::filesystem::path svg_path = out_dir / "smoke.svg";
  const std::filesystem::path pdf_path = out_dir / "smoke.pdf";
  book.save_svg(svg_path.string());
  book.save_pdf(pdf_path.string());
  assert(std::filesystem::exists(svg_path));
  assert(std::filesystem::exists(pdf_path));
  assert(std::filesystem::file_size(svg_path) > 0);
  assert(std::filesystem::file_size(pdf_path) > 0);
  return 0;
}
