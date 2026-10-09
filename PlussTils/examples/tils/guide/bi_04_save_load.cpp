#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace fs = std::filesystem;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  namespace bi = pluss::bi;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}, {3}, cfg))},
  });
  const pluss::bundle::Nexus nexus({{"sales", sales}});

  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 2);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).title("Rows").place(0, 0, 1, 2));
  studio.set_filter(bi::Filter{
      .key = "region_is_north",
      .feature = "region",
      .op = bi::FilterOp::Eq,
      .payload = bi::FilterScalar{std::string("north")},
  });

  // Save the studio to a private file, then load it back against the same data.
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_save_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  const fs::path saved = dir / "studio.bi";
  studio.save(saved.string());
  const bi::Studio restored = bi::Studio::load(saved.string(), nexus);

  std::cout << "boards restored: " << restored.boards().size() << '\n';
  std::cout << "board title restored: " << (restored.boards().front().title() == "Sales overview") << '\n';
  std::cout << "filter restored: " << restored.session().has_filter("region_is_north") << '\n';
  std::cout << "restored rows under the filter: "
            << restored.render().boards.front().tiles.front().table->height() << '\n';
  fs::remove_all(dir);
}
