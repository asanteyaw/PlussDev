#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

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
  namespace bi = pluss::bi;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}, {3}, cfg))},
  });
  const pluss::bundle::Nexus nexus({{"sales", sales}});

  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Quarterly sales").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0, 1, 1));

  // Export a standalone HTML document. The file is written under a private directory.
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_html_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  const fs::path target = dir / "quarterly.html";
  studio.export_html(target.string(), "Quarterly sales", 900, 600);

  std::ifstream in(target);
  const std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::cout << "file exists: " << fs::exists(target) << '\n';
  std::cout << "document has an html root: " << (html.find("<html") != std::string::npos) << '\n';
  std::cout << "document carries the export title: " << (html.find("Quarterly sales") != std::string::npos) << '\n';
  std::cout << "document is not empty: " << (html.size() > 1000) << '\n';
  fs::remove_all(dir);
}
