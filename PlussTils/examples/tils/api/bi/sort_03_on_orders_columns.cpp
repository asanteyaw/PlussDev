#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Six sales rows. Regions: north, south, north, east, south, north. Months: jan, jan, feb, feb, mar, mar.
// Amounts 10 to 60, units 1 to 6, and one date per row from January to June 2024.
// Four months of sales. The region of the third row is missing, so a pivot on region has a missing column.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  // Regions in first-seen order: south, north, missing, east.
  const DataFrame sales({
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb"}))},
      {"region", Slate(pluss::make_note(std::vector<std::string>{"south", "north", "NULL", "east"}),
                        pluss::make_series(std::vector<bool>{true, true, false, true}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40}, {4}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The column names of the pivot the studio renders, in the order the studio keeps them.
std::string pivot_columns(bool dropna, bool sort) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::pivot("pv").pivot_index({"month"}).pivot_columns({"region"}).pivot_values({"amount"})
      .op(pluss::table::GroupOp::Sum).dropna(dropna).sort(sort).query(bi::from("sales")).place(0, 0));
  const auto tile = studio.render().boards.front().tiles.front();
  std::string out;
  for (const auto& name : tile.table->features()) out += name + " ";
  return out;
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_view_flag_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}

// The page of a bar with the given flags. A bar does not use dropna or sort, so its page should not depend on them.
std::string bar_page(bool dropna, bool sort) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::bar("by").x("region").y("amount").dropna(dropna).sort(sort).query(bi::from("sales")).place(0, 0));
  return page_text(studio);
}

int main() {
  std::cout << "pivot columns with sort on: " << pivot_columns(true, true) << '\n';
  return 0;
}
