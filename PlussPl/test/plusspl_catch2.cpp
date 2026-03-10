#include <pluss/pluss.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
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
  book.legend(KeyDock::East).x_caption("terrain", "shared x");

  auto& pane2 = book.pane2("signals");
  pane2.grid(GridStyle::MajorMinor)
      .scale_x(ScalePolicy::log10())
      .place(0, 0)
      .x_label("frequency")
      .y_label("gain")
      .x_ticks(std::array<double, 3>{1.0, 10.0, 100.0},
               std::vector<std::string>{"low", "mid", "high"})
      .legend(KeyPlace::NorthEast)
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
  pane3.grid(GridStyle::Major).place(0, 1, 2, 1).x_label("x").y_label("y").z_label("z").legend(KeyPlace::NorthEast).scale_guide();
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

TEST_CASE("PlussPl exposes font scaling and rotation controls on panes", "[plusspl][axis][style]") {
  using namespace pluss::pl;

  Studio book("axis-style-controls");
  auto& pane = book.pane3();
  pane.tick_font_scale(1.3)
      .axis_label_font_scale(1.2)
      .x_tick_rotation(60.0)
      .y_tick_rotation(120.0)
      .z_tick_rotation(210.0)
      .x_label_rotation(90.0)
      .y_label_rotation(0.0)
      .z_label_rotation(180.0);

  REQUIRE(pane.tick_font_scale() == 1.3);
  REQUIRE(pane.axis_label_font_scale() == 1.2);
  REQUIRE(pane.x_axis().tick_rotation_deg.has_value());
  REQUIRE(pane.y_axis().tick_rotation_deg.has_value());
  REQUIRE(pane.z_axis().tick_rotation_deg.has_value());
  REQUIRE(pane.x_axis().label_rotation_deg.has_value());
  REQUIRE(pane.y_axis().label_rotation_deg.has_value());
  REQUIRE(pane.z_axis().label_rotation_deg.has_value());

  pane.clear_x_tick_rotation()
      .clear_y_tick_rotation()
      .clear_z_tick_rotation()
      .clear_x_label_rotation()
      .clear_y_label_rotation()
      .clear_z_label_rotation();

  REQUIRE(!pane.x_axis().tick_rotation_deg.has_value());
  REQUIRE(!pane.y_axis().tick_rotation_deg.has_value());
  REQUIRE(!pane.z_axis().tick_rotation_deg.has_value());
  REQUIRE(!pane.x_axis().label_rotation_deg.has_value());
  REQUIRE(!pane.y_axis().label_rotation_deg.has_value());
  REQUIRE(!pane.z_axis().label_rotation_deg.has_value());

  REQUIRE_THROWS(pane.tick_font_scale(0.0));
  REQUIRE_THROWS(pane.axis_label_font_scale(-1.0));
  REQUIRE_THROWS(pane.x_tick_rotation(std::numeric_limits<double>::quiet_NaN()));
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

TEST_CASE("PlussPl normalizes TeX-style text in labels and ticks", "[plusspl][text][tex]") {
  using namespace pluss::pl;

  Studio book("tex-normalization");
  auto& pane = book.pane2();
  pane.x_label("$\\alpha + \\beta \\leq \\gamma$")
      .y_label("u \\geq 0")
      .x_ticks(std::array<double, 2>{0.0, 1.0}, std::vector<std::string>{"t \\leq 1", "t \\geq 1"});
  pane.trend(std::array<double, 2>{0.0, 1.0}, std::array<double, 2>{0.0, 1.0});

  const std::string svg = book.to_svg();

  REQUIRE(svg.find("\\alpha") == std::string::npos);
  REQUIRE(svg.find("\\beta") == std::string::npos);
  REQUIRE(svg.find("\\leq") == std::string::npos);
  REQUIRE(svg.find("\\geq") == std::string::npos);

  Studio pdf_book("pdf-unicode-fallback");
  auto& pdf_pane = pdf_book.pane2();
  pdf_pane.x_label("A\xE2\x89\xA4" "B")
      .y_label("C\xE2\x89\xA5" "D");
  pdf_pane.trend(std::array<double, 2>{0.0, 1.0}, std::array<double, 2>{0.0, 1.0});

  const std::string pdf = pdf_book.to_pdf();
  REQUIRE(pdf.find("<=") != std::string::npos);
  REQUIRE(pdf.find(">=") != std::string::npos);
}

TEST_CASE("PlussPl flash facade maps flash-like names to PlussPl traces", "[plusspl][flash]") {
  using namespace pluss::pl;
  using namespace pluss::pl::flash;

  Studio book("flash-facade");
  auto& pane = book.pane2("main");
  set_theme(book, Theme::paper());
  set_style(pane, GridStyle::Major);
  set_context(book, 1000.0, 700.0, 1.2, 1.1);

  auto x = pluss::linspace(0.0, 1.0, 8, pluss::ScalarType::Float64);
  auto y = pluss::make_series(std::vector<double>{0.0, 0.2, 0.35, 0.6, 0.7, 0.82, 0.9, 1.0}, {8});

  scatterplot(pane, x, y).label("scatter");
  lineplot(pane, x, y).label("line");
  displot(pane, y, 6).label("dis");
  histplot(pane, y, 6).label("hist");
  regplot(pane, x, y).label("reg");
  residplot(pane, x, y).label("resid");
  ecdfplot(pane, y).label("ecdf");
  kdeplot(pane, y, 32).label("kde");
  rugplot(pane, y, 0.04).label("rug");

  auto cats = pluss::make_note({"A", "B", "A", "C", "B", "A", "C", "A"});
  auto cat_vals = pluss::make_series(std::vector<double>{0.3, 0.5, 0.42, 0.65, 0.58, 0.35, 0.72, 0.4}, {8});
  auto cat_err = pluss::make_series(std::vector<double>{0.04, 0.05, 0.03, 0.06, 0.05, 0.04, 0.05, 0.03}, {8});
  auto hue_note = pluss::make_note({"alpha", "beta", "gamma", "alpha", "beta", "gamma", "alpha", "beta"});
  auto hue_num = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 0.0, 1.0, 2.0, 0.0, 1.0}, {8});
  stripplot(pane, cats, cat_vals).label("strip");
  swarmplot(pane, cats, cat_vals).label("swarm");
  boxplot(pane, cats, cat_vals).label("box");
  violinplot(pane, cats, cat_vals).label("violin");
  boxenplot(pane, cat_vals).label("boxen");
  pointplot(pane, x, y, y).label("point");
  barplot(pane, x, y).label("bar");
  catplot(pane, cats, cat_vals, "violin").label("cat");
  countplot(pane, cats).label("counts");
  scatterplot(pane, x, y, hue_note);
  lineplot(pane, x, y, hue_note);
  relplot(pane, x, y, hue_num, "scatter");
  relplot(pane, x, y, hue_num, "line");

  auto z = pluss::make_series(
      std::vector<double>{
          0.0, 0.4, 0.7,
          0.2, 0.6, 0.9,
          0.3, 0.8, 1.0,
      },
      {3, 3});
  auto& heat = book.pane2("heat");
  heatmap(heat, std::array<double, 3>{1.0, 2.0, 3.0}, std::array<double, 3>{1.0, 2.0, 3.0}, z).label("heatmap");
  clustermap(heat, std::array<double, 3>{1.0, 2.0, 3.0}, std::array<double, 3>{1.0, 2.0, 3.0}, z).label("cluster");

  REQUIRE(book.pane_count() == 2);
  REQUIRE(book.panes().front().trace_count() >= 31);
  REQUIRE(book.panes().back().trace_count() >= 2);
  std::string trace_labels;
  for (const auto& tr : book.panes().front().traces()) {
    if (tr.label_text().empty()) {
      continue;
    }
    if (!trace_labels.empty()) {
      trace_labels += " | ";
    }
    trace_labels += tr.label_text();
  }
  const std::string svg = book.to_svg();
  CAPTURE(trace_labels);
  CAPTURE(svg.find("... +"), svg.find("hue=1"), svg.find("alpha"));
  {
    std::ofstream debug("plusspl_flash_facade_debug.svg");
    debug << svg;
  }
  const std::size_t more_pos = svg.find("... +");
  if (more_pos != std::string::npos) {
    const std::size_t preview_len = std::min<std::size_t>(180, svg.size() - more_pos);
    CAPTURE(svg.substr(more_pos, preview_len));
  }
  REQUIRE(svg.find("<svg") != std::string::npos);
  REQUIRE(svg.find("alpha") != std::string::npos);
  REQUIRE(svg.find("hue=1") != std::string::npos);

  Studio rel("flash-relplot");
  relplot(rel, x, y, "line", "rel").label("rel-line");
  REQUIRE(rel.pane_count() == 1);
  REQUIRE(rel.panes().front().trace_count() == 1);
  REQUIRE(rel.to_svg().find("<svg") != std::string::npos);

  Studio lm("flash-lmplot");
  lmplot(lm, x, y, "lm");
  REQUIRE(lm.pane_count() == 1);
  REQUIRE(lm.panes().front().trace_count() >= 2);

  Studio facet("flash-facet-grid");
  FacetGrid(facet, 2, 3);
  REQUIRE(facet.mosaic_layout().active);

  Studio pair("flash-pair-grid");
  PairGrid(pair, 3);
  pairplot(pair, {x.reshape({x.numel()}), y.reshape({y.numel()}), cat_vals.reshape({cat_vals.numel()})});
  REQUIRE(pair.to_svg().find("<svg") != std::string::npos);

  Studio joint("flash-joint-grid");
  JointGrid(joint);
  jointplot(joint, x, y, "scatter");
  REQUIRE(joint.to_svg().find("<svg") != std::string::npos);

  REQUIRE(!color_palette("ocean").empty());
  REQUIRE(!hls_palette(5).empty());
  REQUIRE(!husl_palette(5).empty());
  REQUIRE(!cubehelix_palette(5).empty());
  REQUIRE(!light_palette(Rgba::from_u8(42, 95, 153), 5).empty());
  REQUIRE(!dark_palette(Rgba::from_u8(42, 95, 153), 5).empty());
  REQUIRE(!diverging_palette(Rgba::from_u8(42, 95, 153), Rgba::from_u8(211, 78, 60), 7).empty());
  REQUIRE(!blend_palette({Rgba::from_u8(42, 95, 153), Rgba::from_u8(211, 78, 60)}, "blend").empty());
  REQUIRE(!mpl_palette("viridis").empty());
  REQUIRE(!xkcd_palette().empty());
  REQUIRE(!crayon_palette().empty());

  const auto names = get_dataset_names();
  REQUIRE(names.numel() > 0);
  const auto tips = load_dataset("tips");
  REQUIRE(tips.has_value());
  REQUIRE(tips->numeric("tip").has_value());
  REQUIRE(tips->note("day").has_value());
}

TEST_CASE("PlussPl flash exposes direct 3D wrappers and aliases", "[plusspl][flash][3d]") {
  using namespace pluss::pl;
  using namespace pluss::pl::flash;

  Studio book("flash-3d");
  auto& pane = book.pane3("vol");
  pane.grid(GridStyle::Major);

  auto z = pluss::make_series(
      std::vector<double>{
          0.0, 0.4, 0.7,
          0.2, 0.6, 0.9,
          0.3, 0.8, 1.0,
      },
      {3, 3});
  auto c = pluss::make_series(
      std::vector<double>{
          0.1, 0.3, 0.5,
          0.2, 0.45, 0.62,
          0.32, 0.58, 0.84,
      },
      {3, 3});
  auto iso = pluss::make_series(
      std::vector<double>{
          0.0, 0.1, 0.2, 0.1, 0.5, 0.8, 0.2, 0.8, 1.2,
          0.1, 0.3, 0.6, 0.3, 0.9, 1.3, 0.6, 1.3, 1.8,
          0.2, 0.4, 0.7, 0.4, 1.0, 1.4, 0.7, 1.4, 2.0,
      },
      {3, 3, 3});

  path3(pane, std::array<double, 4>{0.0, 0.5, 1.0, 1.5},
        std::array<double, 4>{0.0, 0.4, 0.9, 1.4},
        std::array<double, 4>{0.2, 0.45, 0.85, 1.25}).label("path3");
  cloud3(pane, std::array<double, 4>{0.1, 0.6, 1.1, 1.4},
         std::array<double, 4>{0.1, 0.5, 0.95, 1.3},
         std::array<double, 4>{0.3, 0.62, 0.98, 1.4}).label("cloud3");
  sheet(pane, std::array<double, 3>{0.0, 1.0, 2.0},
        std::array<double, 3>{0.0, 1.0, 2.0},
        z, c).label("sheet");
  lattice(pane, std::array<double, 3>{0.0, 1.0, 2.0},
          std::array<double, 3>{0.0, 1.0, 2.0},
          z).label("lattice");
  ridge3(pane, std::array<double, 3>{0.0, 1.0, 2.0},
         std::array<double, 3>{0.0, 1.0, 2.0},
         z, 6).label("ridge3");
  spire3(pane, std::array<double, 3>{0.0, 1.0, 2.0},
         std::array<double, 3>{0.0, 1.0, 2.0},
         z).label("spire3");
  isoshell(pane,
           std::array<double, 3>{0.0, 1.0, 2.0},
           std::array<double, 3>{0.0, 1.0, 2.0},
           std::array<double, 3>{0.0, 1.0, 2.0},
           iso,
           4).label("isoshell");
  plot3D(pane, std::array<double, 4>{0.0, 0.5, 1.0, 1.5},
         std::array<double, 4>{0.0, 0.4, 0.9, 1.4},
         std::array<double, 4>{0.2, 0.45, 0.85, 1.25}).label("plot3D");
  scatter3D(pane, std::array<double, 4>{0.1, 0.6, 1.1, 1.4},
            std::array<double, 4>{0.1, 0.5, 0.95, 1.3},
            std::array<double, 4>{0.3, 0.62, 0.98, 1.4}).label("scatter3D");
  plot_surface(pane, std::array<double, 3>{0.0, 1.0, 2.0},
               std::array<double, 3>{0.0, 1.0, 2.0},
               z).label("plot_surface");
  plot_wireframe(pane, std::array<double, 3>{0.0, 1.0, 2.0},
                 std::array<double, 3>{0.0, 1.0, 2.0},
                 z).label("plot_wireframe");
  contour3D(pane, std::array<double, 3>{0.0, 1.0, 2.0},
            std::array<double, 3>{0.0, 1.0, 2.0},
            z, 5).label("contour3D");
  bar3d(pane, std::array<double, 3>{0.0, 1.0, 2.0},
        std::array<double, 3>{0.0, 1.0, 2.0},
        z).label("bar3d");

  REQUIRE(book.pane_count() == 1);
  REQUIRE(book.panes().front().trace_count() >= 13);
  REQUIRE(book.to_svg().find("<svg") != std::string::npos);
}

TEST_CASE("PlussPl Studio show writes html payload", "[plusspl][display]") {
  using namespace pluss::pl;

  Studio book("show-html");
  auto& pane = book.pane2();
  pane.trend(std::array<double, 3>{0.0, 1.0, 2.0}, std::array<double, 3>{0.2, 0.5, 0.8});

  REQUIRE(book.show("Show HTML", 900, 620, false));
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
