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


// The text of the first kpi value on the page.
std::string kpi_value(const bi::Studio& studio) {
  const std::string page = page_text(studio);
  const std::string key = "class=\"bi-kpi-value\">";
  const auto at = page.find(key);
  if (at == std::string::npos) return "none";
  const auto end = page.find('<', at + key.size());
  return page.substr(at + key.size(), end - at - key.size());
}

int main() {
  // A kpi reduces one feature with the operation on the view. Its value prints in its shortest form unless you set decimals.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::kpi("count").feature("amount").query(bi::from("sales")).place(0, 0));
  std::cout << "count of amounts: " << kpi_value(studio) << '\n';

  bi::Studio sums("sum_studio");
  sums.source(nexus);
  sums.board("overview").grid(1, 1);
  sums.board("overview").view(bi::kpi("sum").feature("amount").op(pluss::table::GroupOp::Sum).query(bi::from("sales")).place(0, 0));
  std::cout << "sum of amounts: " << kpi_value(sums) << '\n';

  bi::Studio means("mean_studio");
  means.source(nexus);
  means.board("overview").grid(1, 1);
  means.board("overview").view(bi::kpi("mean").feature("units").op(pluss::table::GroupOp::Mean).query(bi::from("sales")).place(0, 0));
  std::cout << "mean of units, shortest: " << kpi_value(means) << '\n';

  bi::Studio fixed("fixed_studio");
  fixed.source(nexus);
  fixed.board("overview").grid(1, 1);
  fixed.board("overview").view(bi::kpi("mean").feature("units").op(pluss::table::GroupOp::Mean).decimals(2).query(bi::from("sales")).place(0, 0));
  std::cout << "mean of units, two decimals: " << kpi_value(fixed) << '\n';

  // A kpi with no feature has nothing to reduce, so the studio refuses it when it renders.
  bi::Studio refused("refused_studio");
  refused.source(nexus);
  refused.board("overview").grid(1, 1);
  refused.board("overview").view(bi::kpi("headline").query(bi::from("sales")).place(0, 0));
  bool was_refused = false;
  try {
    (void)refused.render();
  } catch (const std::exception&) {
    was_refused = true;
  }
  std::cout << "kpi with no feature refused: " << was_refused << '\n';
  return 0;
}
