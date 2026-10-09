#include <pluss/bi/bi.h>
#include <pluss/bi/serve.h>
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
  // start_serving runs the dashboard in the background and returns a handle. show writes the page and opens nothing when launch is false.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));

  auto handle = studio.start_serving("Sales", 900, 600, 0, false, bi::DashboardChrome::None);
  std::cout << "running before stop: " << handle.running() << '\n';
  handle.stop();
  handle.wait();
  std::cout << "running after stop: " << handle.running() << '\n';

  std::cout << "show without launch succeeds: " << studio.show("Sales overview", 900, 600, false, bi::DashboardChrome::None) << '\n';
  return 0;
}
