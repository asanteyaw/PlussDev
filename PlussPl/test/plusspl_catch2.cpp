#include <pluss/pluss.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

namespace {

[[nodiscard]] std::optional<double> text_x_by_label(const std::string& svg, std::string_view label) {
  const std::string needle = ">" + std::string(label) + "</text>";
  const std::size_t text_close = svg.find(needle);
  if (text_close == std::string::npos) {
    return std::nullopt;
  }
  const std::size_t text_open = svg.rfind("<text ", text_close);
  if (text_open == std::string::npos) {
    return std::nullopt;
  }
  const std::size_t x_attr = svg.find("x=\"", text_open);
  if (x_attr == std::string::npos || x_attr > text_close) {
    return std::nullopt;
  }
  const std::size_t value_start = x_attr + 3;
  const std::size_t value_end = svg.find('"', value_start);
  if (value_end == std::string::npos) {
    return std::nullopt;
  }
  try {
    return std::stod(svg.substr(value_start, value_end - value_start));
  } catch (...) {
    return std::nullopt;
  }
}

} // namespace

TEST_CASE("PlussPl studio exports SVG and PDF", "[plusspl][export]") {
  using namespace pluss::pl;

  Studio book("core-smoke", Theme::ocean());
  book.size(960.0, 720.0);
  book.mosaic(2, 2, 18.0, 16.0);
  book.headline("Core studio");
  book.trace_key(KeyDock::East).x_caption("terrain", "shared x");

  auto& pane2 = book.pane2("signals");
  pane2.grid(GridStyle::MajorMinor)
      .scale_x(ScalePolicy::log10())
      .place(0, 0)
      .x_label("frequency")
      .y_label("gain")
      .x_ticks(std::array<double, 3>{1.0, 10.0, 100.0},
               std::vector<std::string>{"low", "mid", "high"})
      .key(KeyPlace::NorthEast)
      .caption("signal pane")
      .rule_x(10.0, "pivot")
      .band_y({0.55, 1.0}, "target")
      .scale_guide();

  pane2.trend(std::vector<double>{1.0, 10.0, 100.0}, std::array<double, 3>{0.5, 0.8, 1.2})
      .label("response")
      .width(2.0F);
  pane2.radial(std::array<double, 3>{0.0, 1.0, 2.0}, std::array<double, 3>{1.0, 0.5, 1.5})
      .label("radial");
  pane2.cloud(std::array<double, 3>{1.0, 10.0, 100.0},
              std::array<double, 3>{0.6, 0.9, 1.0},
              std::array<double, 3>{20.0, 28.0, 36.0},
              std::array<double, 3>{0.1, 0.6, 0.9})
      .label("cloud");
  pane2.ribbon(std::array<double, 3>{1.0, 10.0, 100.0},
               std::array<double, 3>{0.2, 0.35, 0.55},
               0.1)
      .label("ribbon");
  pane2.whisk(std::array<double, 3>{1.0, 10.0, 100.0},
              std::array<double, 3>{0.55, 0.82, 1.1},
              std::array<double, 3>{0.08, 0.05, 0.12},
              std::array<double, 3>{0.1, 0.07, 0.15})
      .label("whisk");
  pane2.spike(std::array<double, 3>{2.0, 4.0, 8.0},
              std::array<double, 3>{0.35, 0.65, 0.92})
      .label("spike");
  pane2.wedge(std::array<double, 4>{4.0, 3.0, 2.0, 1.0},
              std::array<double, 4>{0.0, 1.0, 0.0, 0.0},
              std::vector<std::string>{"a", "b", "c", "d"})
      .label("wedge");
  auto contour_z = pluss::make_series(
      std::vector<double>{
          -1.0, -0.3, 0.6,
          -0.2, 0.5, 1.1,
          0.2,  0.9, 1.5,
      },
      {3, 3});
  pane2.fieldmap(std::array<double, 3>{-1.0, 0.0, 1.0},
                 std::array<double, 3>{-1.0, 0.0, 1.0},
                 contour_z)
      .label("fieldmap");
  pane2.contour(std::array<double, 3>{-1.0, 0.0, 1.0},
                std::array<double, 3>{-1.0, 0.0, 1.0},
                contour_z,
                std::array<double, 3>{-0.25, 0.5, 1.0})
      .label("topography")
      .width(1.1F)
      .annotate_levels();
  pane2.isoband(std::array<double, 3>{-1.0, 0.0, 1.0},
                std::array<double, 3>{-1.0, 0.0, 1.0},
                contour_z,
                std::array<double, 3>{-0.25, 0.5, 1.0})
      .label("bands");
  pane2.pin("crest", 1.0, 1.5);
  pane2.columns(std::array<double, 3>{2.0, 4.0, 8.0}, std::array<double, 3>{0.4, 0.7, 1.1})
      .label("columns");
  pane2.bucket(std::array<double, 7>{-1.0, -0.4, -0.2, 0.1, 0.3, 0.8, 1.2}, 5)
      .label("bucket");
  pane2.quartet(std::array<double, 6>{1, 1, 1, 2, 2, 2},
                std::array<double, 6>{0.2, 0.3, 0.5, 0.8, 1.0, 1.4})
      .label("quartet");
  pane2.violin(std::array<double, 8>{1, 1, 1, 1, 2, 2, 2, 2},
               std::array<double, 8>{0.15, 0.25, 0.3, 0.6, 0.75, 0.85, 1.1, 1.35})
      .label("violin");

  auto& pane3 = book.pane3("surface");
  pane3.grid(GridStyle::Major).place(0, 1, 2, 1).x_label("x").y_label("y").z_label("z").key(KeyPlace::NorthEast).scale_guide();
  pane3.x_ticks(std::array<double, 2>{0.0, 1.0},
                std::vector<std::string>{"near", "far"});
  pane3.y_ticks(std::array<double, 2>{0.0, 1.0},
                std::vector<std::string>{"left", "right"});
  pane3.z_ticks(std::array<double, 3>{0.0, 1.5, 3.0},
                std::vector<std::string>{"floor", "mid", "crest"});
  pane3.align_xy("terrain").rule_z(1.0, "slice").band_z({0.2, 0.8}, "window");
  pane3.lens().spin(12.0, 8.0);
  pane3.lights().sun_key();

  auto z = pluss::make_series(std::vector<double>{0.0, 1.0, 1.5, 3.0}, {2, 2});
  auto c = pluss::make_series(std::vector<double>{0.1, 0.4, 0.6, 0.9}, {2, 2});
  pane3.sheet(std::array<double, 2>{0.0, 1.0}, std::array<double, 2>{0.0, 1.0}, z, c)
      .label("sheet")
      .shade(ShadeMode::Smooth)
      .lighting(LightingMode::Gouraud)
      .material(MaterialProfile::Shiny)
      .palette(PaletteRamp::lagoon());
  pane3.lattice(std::array<double, 2>{0.0, 1.0}, std::array<double, 2>{0.0, 1.0}, z);
  pane3.cloud3(std::array<double, 3>{0.0, 0.4, 0.8},
               std::array<double, 3>{0.2, 0.6, 1.0},
               std::array<double, 3>{0.6, 1.4, 2.5},
               std::array<double, 3>{12.0, 18.0, 22.0},
               std::array<double, 3>{0.1, 0.5, 0.9})
      .label("cloud3");
  pane3.spire3(std::array<double, 2>{0.0, 1.0},
               std::array<double, 2>{0.0, 1.0},
               z,
               c)
      .label("spire3");
  pane3.ridge3(std::array<double, 2>{0.0, 1.0},
               std::array<double, 2>{0.0, 1.0},
               z,
               std::array<double, 2>{0.75, 2.25})
      .label("ridge")
      .annotate_levels();
  auto iso = pluss::make_series(
      std::vector<double>{
          0.0, 0.1, 0.2, 0.1, 0.5, 0.8, 0.2, 0.8, 1.2,
          0.1, 0.3, 0.6, 0.3, 0.9, 1.3, 0.6, 1.3, 1.8,
          0.2, 0.4, 0.7, 0.4, 1.0, 1.4, 0.7, 1.4, 2.0,
      },
      {3, 3, 3});
  pane3.isoshell(std::array<double, 3>{0.0, 1.0, 2.0},
                 std::array<double, 3>{0.0, 1.0, 2.0},
                 std::array<double, 3>{0.0, 1.0, 2.0},
                 iso,
                 std::array<double, 2>{0.85, 1.45})
      .label("isoshell")
      .shade(ShadeMode::Smooth)
      .transparent();
  pane3.pin("focus", Vec3d{1.0, 1.0, 3.0});

  const std::string svg = book.to_svg();
  const std::string pdf = book.to_pdf();

  REQUIRE(svg.find("<svg") != std::string::npos);
  REQUIRE(svg.find("<path") != std::string::npos);
  REQUIRE(svg.find("Core studio") != std::string::npos);
  REQUIRE(svg.find("topography") != std::string::npos);
  REQUIRE(svg.find("bands") != std::string::npos);
  REQUIRE(svg.find("ridge") != std::string::npos);
  REQUIRE(svg.find("cloud") != std::string::npos);
  REQUIRE(svg.find("cloud3") != std::string::npos);
  REQUIRE(svg.find("fieldmap") != std::string::npos);
  REQUIRE(svg.find("ribbon") != std::string::npos);
  REQUIRE(svg.find("whisk") != std::string::npos);
  REQUIRE(svg.find("spike") != std::string::npos);
  REQUIRE(svg.find("spire3") != std::string::npos);
  REQUIRE(svg.find("isoshell") != std::string::npos);
  REQUIRE(svg.find("wedge") != std::string::npos);
  REQUIRE(svg.find("columns") != std::string::npos);
  REQUIRE(svg.find("bucket") != std::string::npos);
  REQUIRE(svg.find("quartet") != std::string::npos);
  REQUIRE(svg.find("violin") != std::string::npos);
  REQUIRE(svg.find("low") != std::string::npos);
  REQUIRE(svg.find("near") != std::string::npos);
  REQUIRE(svg.find("left") != std::string::npos);
  REQUIRE(svg.find("crest") != std::string::npos);
  REQUIRE(svg.find("trace key") != std::string::npos);
  REQUIRE(svg.find("pivot") != std::string::npos);
  REQUIRE(svg.find("focus") != std::string::npos);
  REQUIRE(pdf.rfind("%PDF-1.4", 0) == 0);

  const auto out_dir = std::filesystem::temp_directory_path() / "plusspl_catch2";
  std::filesystem::create_directories(out_dir);
  const auto svg_path = out_dir / "smoke.svg";
  const auto pdf_path = out_dir / "smoke.pdf";
  book.save_svg(svg_path.string());
  book.save_pdf(pdf_path.string());

  REQUIRE(std::filesystem::exists(svg_path));
  REQUIRE(std::filesystem::exists(pdf_path));
  REQUIRE(std::filesystem::file_size(svg_path) > 0);
  REQUIRE(std::filesystem::file_size(pdf_path) > 0);
}

TEST_CASE("PlussPl surface validation rejects degenerate inputs", "[plusspl][validation]") {
  using namespace pluss::pl;

  Studio book("validation");
  auto& pane = book.pane3();
  auto bad_z = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0}, {3});

  REQUIRE_THROWS(pane.sheet(std::array<double, 3>{0.0, 1.0, 2.0},
                            std::array<double, 3>{0.0, 1.0, 2.0},
                            bad_z));

  auto z = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {2, 2});
  REQUIRE_THROWS(pane.sheet(std::array<double, 3>{0.0, 1.0, 2.0},
                            std::array<double, 2>{0.0, 1.0},
                            z));
}

TEST_CASE("PlussPl contour validation rejects incompatible grids", "[plusspl][validation][contour]") {
  using namespace pluss::pl;

  Studio book("contour-validation");
  auto& pane = book.pane2();
  auto z = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {2, 2});

  REQUIRE_THROWS(pane.contour(std::array<double, 3>{0.0, 1.0, 2.0},
                              std::array<double, 2>{0.0, 1.0},
                              z));

  REQUIRE_THROWS(pane.contour(std::array<double, 2>{0.0, 1.0},
                              std::array<double, 2>{0.0, 1.0},
                              z,
                              0));
}

TEST_CASE("PlussPl field and spire validation reject incompatible grids", "[plusspl][validation][field]") {
  using namespace pluss::pl;

  Studio book("field-validation");
  auto& pane2 = book.pane2();
  auto& pane3 = book.pane3();
  auto z = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {2, 2});

  REQUIRE_THROWS(pane2.fieldmap(std::array<double, 3>{0.0, 1.0, 2.0},
                                std::array<double, 2>{0.0, 1.0},
                                z));
  REQUIRE_THROWS(pane3.spire3(std::array<double, 3>{0.0, 1.0, 2.0},
                              std::array<double, 2>{0.0, 1.0},
                              z));

  auto bad_iso = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {2, 2});
  REQUIRE_THROWS(pane3.isoshell(std::array<double, 2>{0.0, 1.0},
                                std::array<double, 2>{0.0, 1.0},
                                std::array<double, 2>{0.0, 1.0},
                                bad_iso));
  auto iso = pluss::make_series(std::vector<double>(8, 0.25), {2, 2, 2});
  REQUIRE_THROWS(pane3.isoshell(std::array<double, 3>{0.0, 1.0, 2.0},
                                std::array<double, 2>{0.0, 1.0},
                                std::array<double, 2>{0.0, 1.0},
                                iso,
                                0));
}

TEST_CASE("PlussPl statistical mark validation rejects degenerate inputs", "[plusspl][validation][marks]") {
  using namespace pluss::pl;

  Studio book("marks-validation");
  auto& pane = book.pane2();

  REQUIRE_THROWS(pane.bucket(std::array<double, 0>{}, 5));
  REQUIRE_THROWS(pane.bucket(std::array<double, 3>{0.0, 1.0, 2.0}, 0));
  REQUIRE_THROWS(pane.quartet(std::array<double, 2>{1.0, 2.0},
                              std::array<double, 3>{0.2, 0.3, 0.4}));
  REQUIRE_THROWS(pane.violin(std::array<double, 2>{1.0, 2.0},
                             std::array<double, 3>{0.2, 0.3, 0.4}));
}

TEST_CASE("PlussPl layout and uncertainty validation reject degenerate inputs", "[plusspl][validation][layout]") {
  using namespace pluss::pl;

  Studio book("layout-validation");
  book.mosaic(1, 1);
  auto& left = book.pane2("left");
  auto& right = book.pane2("right");

  left.place(0, 0);
  right.place(0, 0);

  REQUIRE_THROWS(book.to_svg());

  Studio whisk_book("whisk-validation");
  auto& pane = whisk_book.pane2();
  REQUIRE_THROWS(pane.whisk(std::array<double, 2>{1.0, 2.0},
                            std::array<double, 3>{0.2, 0.3, 0.4},
                            std::array<double, 3>{0.1, 0.1, 0.1}));
  REQUIRE_THROWS(pane.ribbon(std::array<double, 2>{1.0, 2.0},
                             pluss::make_series(std::vector<double>{0.1, 0.2, 0.3, 0.4}, {4, 1})));
  REQUIRE_THROWS(pane.wedge(std::array<double, 2>{0.0, 0.0}));
  REQUIRE_THROWS(pane.x_ticks(std::array<double, 2>{1.0, 2.0},
                              std::vector<std::string>{"only one"}));
}

TEST_CASE("PlussPl axis policy validation rejects invalid scales and ranges", "[plusspl][validation][axis]") {
  using namespace pluss::pl;

  Studio book("axis-validation");
  auto& pane = book.pane2();

  REQUIRE_THROWS(pane.scale_x(ScalePolicy::symlog(1.0, 1.0)));
  REQUIRE_THROWS(pane.scale_x(ScalePolicy::symlog(10.0, 0.0)));

  pane.scale_x(ScalePolicy::log10());
  REQUIRE_THROWS(pane.x_limits({-1.0, 10.0}));
  REQUIRE_THROWS(pane.x_limits({2.0, 2.0}));

  REQUIRE_THROWS(pane.band_y({3.0, 1.0}));
  REQUIRE_THROWS(pane.rule_x(std::numeric_limits<double>::infinity()));
  REQUIRE_THROWS(pane.pin("bad", std::numeric_limits<double>::quiet_NaN(), 1.0));
}

TEST_CASE("PlussPl exposes limit fit policies on pane axes", "[plusspl][axis][limit-fit]") {
  using namespace pluss::pl;

  Studio book("limit-fit");
  auto& pane = book.pane3();
  pane.x_limit_method(LimitMethod::Padded)
      .y_limit_method(LimitMethod::Tight)
      .z_limit_method(LimitMethod::TickAligned)
      .color_limit_method(LimitMethod::Padded);

  REQUIRE(pane.x_axis().limit_method == LimitMethod::Padded);
  REQUIRE(pane.y_axis().limit_method == LimitMethod::Tight);
  REQUIRE(pane.z_axis().limit_method == LimitMethod::TickAligned);
  REQUIRE(pane.color_axis().limit_method == LimitMethod::Padded);
}

TEST_CASE("PlussPl renders mixed-sign symlog traces", "[plusspl][scale][symlog]") {
  using namespace pluss::pl;

  Studio book("symlog-render");
  auto& pane = book.pane2();
  pane.scale_x(ScalePolicy::symlog(10.0, 0.25)).grid(GridStyle::Major);
  pane.trend(std::array<double, 7>{-100.0, -10.0, -1.0, 0.0, 1.0, 10.0, 100.0},
             std::array<double, 7>{3.0, 2.0, 1.0, 0.0, 1.0, 2.0, 3.0});

  const auto svg = book.to_svg();
  REQUIRE(svg.find("<svg") != std::string::npos);
}

TEST_CASE("PlussPl renders major-minor guides for planar and volume panes", "[plusspl][grid][minor]") {
  using namespace pluss::pl;

  Studio book("minor-guides");
  book.mosaic(1, 2);

  auto& pane2 = book.pane2("planar");
  pane2.place(0, 0)
      .grid(GridStyle::MajorMinor)
      .scale_x(ScalePolicy::log10())
      .scale_y(ScalePolicy::symlog(10.0, 0.2));
  pane2.trend(std::array<double, 6>{0.1, 0.3, 1.0, 3.0, 10.0, 30.0},
              std::array<double, 6>{-2.0, -1.0, -0.3, 0.4, 1.6, 2.8});

  auto& pane3 = book.pane3("volume");
  pane3.place(0, 1)
      .grid(GridStyle::MajorMinor)
      .scale_x(ScalePolicy::log10())
      .scale_y(ScalePolicy::log10())
      .scale_z(ScalePolicy::symlog(10.0, 0.2));
  auto z = pluss::make_series(
      std::vector<double>{
          -1.0, -0.2, 0.5,
          -0.4,  0.1, 0.8,
           0.2,  0.9, 1.6,
      },
      {3, 3});
  pane3.sheet(std::array<double, 3>{0.1, 1.0, 10.0},
              std::array<double, 3>{0.2, 1.5, 12.0},
              z);

  const std::string svg = book.to_svg();
  std::size_t path_count = 0;
  std::size_t pos = 0;
  while ((pos = svg.find("<path", pos)) != std::string::npos) {
    ++path_count;
    pos += 5;
  }
  REQUIRE(path_count > 60);
}

TEST_CASE("PlussPl keeps planar tick labels visible without grid", "[plusspl][axis][ticks]") {
  using namespace pluss::pl;

  Studio book("planar-ticks-no-grid");
  auto& pane = book.pane2("no-grid");
  pane.grid(GridStyle::None)
      .x_ticks(std::array<double, 3>{1.0, 2.0, 3.0}, std::vector<std::string>{"alpha", "beta", "gamma"})
      .y_ticks(std::array<double, 2>{10.0, 20.0}, std::vector<std::string>{"low", "high"});
  pane.trend(std::array<double, 3>{1.0, 2.0, 3.0}, std::array<double, 3>{10.0, 15.0, 20.0});

  const std::string svg = book.to_svg();
  REQUIRE(svg.find("alpha") != std::string::npos);
  REQUIRE(svg.find("gamma") != std::string::npos);
  REQUIRE(svg.find("high") != std::string::npos);
}

TEST_CASE("PlussPl supports tick formatting and categorical helpers", "[plusspl][axis][format]") {
  using namespace pluss::pl;

  Studio book("tick-format");
  auto& pane = book.pane2("formats");
  pane.grid(GridStyle::Major)
      .x_tick_format(TickFormat::Scientific)
      .y_tick_format(TickFormat::Percent)
      .x_categories({"Q1", "Q2", "Q3", "Q4"});
  pane.trend(std::array<double, 4>{1.0, 2.0, 3.0, 4.0},
             std::array<double, 4>{0.12, 0.24, 0.31, 0.5});
  pane.cloud(std::array<double, 4>{1.0, 2.0, 3.0, 4.0},
             std::array<double, 4>{0.11, 0.22, 0.33, 0.44});

  const std::string svg = book.to_svg();
  REQUIRE(svg.find("Q1") != std::string::npos);
  REQUIRE(svg.find("Q4") != std::string::npos);
  REQUIRE(svg.find("%") != std::string::npos);
}

TEST_CASE("PlussPl renders violin traces for degenerate and grouped samples", "[plusspl][violin]") {
  using namespace pluss::pl;

  Studio book("violin-stability");
  auto& pane = book.pane2("stats");
  pane.grid(GridStyle::Major);
  pane.violin(std::array<double, 5>{3.0, 3.0, 3.0, 3.0, 3.0}).label("flat-sample");
  pane.violin(std::array<double, 8>{1, 1, 1, 1, 2, 2, 2, 2},
              std::array<double, 8>{0.2, 0.35, 0.4, 0.55, 0.7, 0.82, 1.0, 1.25})
      .label("grouped-sample");

  const std::string svg = book.to_svg();
  REQUIRE(svg.find("<path") != std::string::npos);
  REQUIRE(svg.find("nan") == std::string::npos);
  REQUIRE(svg.find("inf") == std::string::npos);
}

TEST_CASE("PlussPl projects volume z ticks without screen-space pinning", "[plusspl][volume][axis]") {
  using namespace pluss::pl;

  Studio book("volume-z-ticks");
  auto& pane = book.pane3("volume");
  pane.grid(GridStyle::Major)
      .x_label("x")
      .y_label("y")
      .z_label("z")
      .z_ticks(std::array<double, 3>{0.0, 1.0, 2.0},
               std::vector<std::string>{"z_lo", "z_mid", "z_hi"});
  pane.lens().look(-37.5, 30.0).project(ProjectionKind::Perspective);

  const auto z = pluss::make_series(
      std::vector<double>{
          0.0, 0.6, 1.1,
          0.4, 1.2, 1.7,
          0.8, 1.5, 2.0,
      },
      {3, 3});
  pane.sheet(std::array<double, 3>{-1.0, 0.0, 1.0},
             std::array<double, 3>{-1.0, 0.0, 1.0},
             z);

  const std::string svg = book.to_svg();
  const auto x_lo = text_x_by_label(svg, "z_lo");
  const auto x_mid = text_x_by_label(svg, "z_mid");
  const auto x_hi = text_x_by_label(svg, "z_hi");
  REQUIRE(x_lo.has_value());
  REQUIRE(x_mid.has_value());
  REQUIRE(x_hi.has_value());

  const std::array<double, 3> xs{*x_lo, *x_mid, *x_hi};
  const auto [min_it, max_it] = std::minmax_element(xs.begin(), xs.end());
  REQUIRE(std::abs(*max_it - *min_it) > 0.05);
}
