#include <pluss/mic.h>
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

int main() {
  std::cout << std::boolalpha;
  namespace fs = std::filesystem;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}, {3}, cfg))},
  });
  const pluss::bundle::Nexus nexus({{"sales", sales}});
  std::cout << "table registered: " << nexus.has("sales") << '\n';

  pluss::bi::Studio studio("sales_studio");
  studio.source(nexus);
  pluss::bi::Board& board = studio.board("overview");
  board.title("Sales overview").grid(1, 2);
  board.view(pluss::bi::table("rows").query(pluss::bi::from("sales")).title("Rows").place(0, 0, 1, 2));
  std::cout << "board tiles: " << board.tiles().size() << '\n';

  const fs::path dir = fs::temp_directory_path() / "pluss_bi_board_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  const fs::path target = dir / "overview.html";
  studio.export_html(target.string(), "Sales overview", 900, 600);
  std::cout << "html written: " << fs::exists(target) << '\n';

  std::ifstream in(target);
  const std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::cout << "html is a document: " << (html.find("<html") != std::string::npos) << '\n';
  fs::remove_all(dir);
}
