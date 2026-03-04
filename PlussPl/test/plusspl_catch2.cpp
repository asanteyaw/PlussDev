#include <pluss/pluss.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

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
  REQUIRE(svg.find("wedge") != std::string::npos);
  REQUIRE(svg.find("columns") != std::string::npos);
  REQUIRE(svg.find("bucket") != std::string::npos);
  REQUIRE(svg.find("quartet") != std::string::npos);
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
}

TEST_CASE("PlussPl statistical mark validation rejects degenerate inputs", "[plusspl][validation][marks]") {
  using namespace pluss::pl;

  Studio book("marks-validation");
  auto& pane = book.pane2();

  REQUIRE_THROWS(pane.bucket(std::array<double, 0>{}, 5));
  REQUIRE_THROWS(pane.bucket(std::array<double, 3>{0.0, 1.0, 2.0}, 0));
  REQUIRE_THROWS(pane.quartet(std::array<double, 2>{1.0, 2.0},
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
