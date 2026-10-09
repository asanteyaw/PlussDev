#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Six sales rows: regions north, south, north, east, south, north; months jan, jan, feb, feb, mar, mar;
// amounts 10 to 60; units 1 to 6; and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_guide_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}


int main() {
  // The grid sets how many rows and columns the board has. Each tile names its cell and its spans.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(2, 2);
  studio.board("overview").view(bi::table("wide").query(bi::from("sales")).title("Wide").place(0, 0, 1, 2));
  studio.board("overview").view(bi::kpi("headline").feature("amount").query(bi::from("sales")).title("Headline").place(1, 0));
  studio.board("overview").view(bi::table("tall").query(bi::from("sales")).title("Tall").place(1, 1).min_height(320));

  const auto render = studio.render();
  const auto& board = render.boards.front();
  std::cout << "grid rows and columns: " << board.rows << " " << board.cols << '\n';
  for (const auto& tile : board.tiles) {
    std::cout << tile.key << " at " << tile.layout.row << "," << tile.layout.col
              << " spans " << tile.layout.row_span << "x" << tile.layout.col_span << '\n';
  }
  std::cout << "tall minimum height: " << board.tiles.back().layout.min_height.value_or(0.0) << '\n';
  return 0;
}
